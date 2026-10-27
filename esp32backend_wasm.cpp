// =====================================================================
// POC C M2 (wasm) — Esp32Backend version QEMU-wasm statique.
// Interface ABSOLUMENT identique a esp32backend.h (le reste de l'app est
// agnostique), seule l'implementation change :
//   - symboles QEMU declares extern "C" et LINKES dans le module
//     (libqemu-all.a fusionnee, cf. /tmp/opencode/m1-merge.sh) — pas de
//     dlopen inapplicable sous wasm ;
//   - QEMU tourne dans le thread QThread::create comme au desktop, mais le
//     corps du thread replique system/main.c 10.1 (qemu_init -> restitution
//     BQL/replay -> qemu_default_main SANS exit()) ;
//   - ROMs prechargees depuis les ressources qrc vers MEMFS /bios
//     (stdio passe par le FS Emscripten) ;
//   - image flash produite par prepareFlashImage (code desktop) puis ecrite
//     dans le MEMFS "/espsimu_flash.bin" (dossier temp natif indisponible).
// =====================================================================
#include "esp32backend.h"
#include "diagnostic.h"
#include "grovelcdrgb.h"

#include <cstring>
#include <cstdlib>
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QMetaObject>
#include <QCoreApplication>
#include <QElapsedTimer>

// ==================== Symboles QEMU lies statiquement ====================
// Signatures verrouillees par l'audit llvm-nm de libqemu-all.a (M1/M2 POC C)
// et les headers du fork qemu 10.1 (system/system.h, main-loop.h, replay.h).
extern "C" {
void qemu_init(int argc, char **argv);
int  qemu_main_loop(void);
void qemu_cleanup(int status);
void bql_lock_impl(const char *file, int line);
void bql_unlock(void);
void replay_mutex_lock(void);
void replay_mutex_unlock(void);
void qemu_system_reset_request(int reason);
void qemu_system_shutdown_request(int reason);
void qemu_system_vmstop_request_prepare(void);
void qemu_system_vmstop_request(int reason);
void vm_start(void);
void qemu_picsimlab_register_callbacks(void *cbs);
void qemu_picsimlab_set_pin(int pin, int value);
void qemu_picsimlab_set_apin_mv(int pin, int mv);
void qemu_picsimlab_uart_receive(int uart, const uint8_t *buf, int len);
uint32_t *qemu_picsimlab_get_internals(int cfg);
int qemu_picsimlab_flash_dump(int64_t offset, void *buf, int bytes);
}
#define bql_lock() bql_lock_impl(__FILE__, __LINE__)

// ==================== Codes get_internals ====================
#define QEMU_INTERNAL_STRAP              0
#define QEMU_INTERNAL_GPIO_IN_SEL        1
#define QEMU_INTERNAL_GPIO_OUT_SEL       2
#define QEMU_INTERNAL_IOMUX_GPIOS        3
#define QEMU_INTERNAL_LEDC_CHANNEL_CONF  4
#define QEMU_INTERNAL_LEDC_TIMER_FREQ    5
#define QEMU_INTERNAL_LEDC_CHANNEL_DUTY  6
#define QEMU_INTERNAL_UART0_BAUD         7
#define QEMU_INTERNAL_UART1_BAUD         8
#define QEMU_INTERNAL_UART2_BAUD         9

// ==================== RunState (include/system/runstate.h) ====================
#define RUN_STATE_PAUSED   4

// ==================== ShutdownCause ====================
#define SHUTDOWN_CAUSE_HOST_QMP_SYSTEM_RESET  3
#define SHUTDOWN_CAUSE_HOST_UI                5
#define SHUTDOWN_CAUSE_GUEST_RESET            7   // utilise par le code QEMU natif ESP32 (esp32_dig_reset, cpu_reset, etc.)

// ==================== Structure callbacks (identique au fork) ====================
typedef struct {
    void (*picsimlab_write_pin)(int pin, int value);
    void (*picsimlab_dir_pin)(int pin, int value);
    int  (*picsimlab_i2c_event)(const uint8_t id, const uint8_t addr, const uint16_t event);
    uint8_t (*picsimlab_spi_event)(const uint8_t id, const uint16_t event);
    void (*picsimlab_uart_tx_event)(const uint8_t id, const uint8_t value);
    const short int *pinmap;
    void (*picsimlab_rmt_event)(const uint8_t channel, const uint32_t config0, const uint32_t value);
} callbacks_t;

// ==================== Pinmap ESP32-DevKitC V4 ====================
static const short int devkitc_pinmap[] = {
    40,
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39
};

// ==================== Pointeur global pour les callbacks C ====================
static Esp32Backend *g_backend = nullptr;

// ==================== Callbacks C appeles par QEMU (thread QEMU !) ====================
extern "C" void on_picsimlab_write_pin(int pin, int value) {
    if (g_backend) g_backend->emitGpioChanged(pin, value);
}
extern "C" void on_picsimlab_dir_pin(int pin, int value) {
    if (g_backend) g_backend->emitGpioDir(pin, value);
}
extern "C" void on_picsimlab_uart_tx(const uint8_t id, const uint8_t value) {
    if (EspDiag::actif()) EspDiag::uartSource(id, value);
    if (g_backend) g_backend->emitUartTx(id, value);
}
extern "C" int on_picsimlab_i2c_event(const uint8_t id, const uint8_t addr, const uint16_t event) {
    if (g_backend) return g_backend->handleI2c(id, addr, event);
    return 0;
}
extern "C" uint8_t on_picsimlab_spi_event(const uint8_t id, const uint16_t event) {
    if (g_backend) emit g_backend->spiEvent(id, event);
    return 0;
}
extern "C" void on_picsimlab_rmt_event(const uint8_t ch, const uint32_t cfg0, const uint32_t val) {
    if (g_backend) emit g_backend->rmtEvent(ch, cfg0, val);
}

// ==================== Constructeur / destructeur ====================
Esp32Backend::Esp32Backend(QObject *parent)
    : QObject(parent),
    libHandle(nullptr),
    qemuThread(nullptr),
    running(false),
    m_ready(false),
    qemuArgv(nullptr),
    qemuArgc(0),
    m_grove(nullptr)
{
    g_backend = this;
}

Esp32Backend::~Esp32Backend()
{
    shutdown();
    freeArgv();
    if (g_backend == this) g_backend = nullptr;
}

// ==================== Chargement de la bibliotheque ====================
// Sous wasm les symboles sont lies statiquement dans le module : rien a
// dlopen. L'appel reste pour la parite du modele (modele.cpp l'appelle).
bool Esp32Backend::loadLibrary(const QString &)
{
    EspDiag::init(nullptr);   // traces sans temps virtuel (pas de dlsym en wasm)
    return true;
}

bool Esp32Backend::resolveSymbols()
{
    // Symboles resolus au link (verification faite par l'audit llvm-nm M1) :
    // qemu_init, qemu_main_loop, qemu_cleanup, bql_*, vm_start, vmstop_*,
    // reset/shutdown_request, qemu_picsimlab_* + shim ring.
    return true;
}

// ==================== Firmware ====================
// Majorette desktop : prepareFlashImage/placeInImage identiques (fichiers
// locaux lisibles via stdio sous MEMFS). Seule l'ecriture de l'image change :
// pas de dossier natif temp, patron fixe "/espsimu_flash.bin" dans le MEMFS.
static const qint64 FLASH_SIZE_DEFAULT = 4 * 1024 * 1024;
static const qint64 OFFSET_BOOTLOADER  = 0x1000;
static const qint64 OFFSET_PARTITIONS  = 0x8000;
static const qint64 OFFSET_BOOT_APP0   = 0xE000;
static const qint64 OFFSET_APP         = 0x10000;

static bool isFlashSize(qint64 size)
{
    return size == 2*1024*1024 || size == 4*1024*1024 ||
           size == 8*1024*1024 || size == 16*1024*1024;
}

static QString findBootApp0(const QString &dir)
{
    if (QFileInfo::exists(dir + "/boot_app0.bin")) return dir + "/boot_app0.bin";
    // Ressource embarquee (octets canoniques de la partition OTA vide 8 Ko)
    if (QFile::exists(":/boot_app0.bin")) return ":/boot_app0.bin";
    return QString();
}

bool Esp32Backend::placeInImage(QByteArray &img, const QString &file, qint64 offset)
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) {
        emit error(QString("Lecture impossible: %1").arg(file));
        return false;
    }
    QByteArray data = f.readAll();
    if (offset + data.size() > img.size()) {
        emit error(QString("%1 trop gros pour la flash de %2 Mo")
                       .arg(QFileInfo(file).fileName()).arg(img.size() / (1024*1024)));
        return false;
    }
    img.replace(int(offset), data.size(), data);
    return true;
}

bool Esp32Backend::loadFirmware(const QString &binPath)
{
    QByteArray img;
    if (!prepareFlashImage(binPath, img)) return false;

    // Ecriture dans le MEMFS (stdio -> FS Emscripten, FORCE_FILESYSTEM requis)
    const QString out = "/espsimu_flash.bin";
    QFile o(out);
    if (!o.open(QIODevice::WriteOnly | QIODevice::Truncate) || o.write(img) != img.size()) {
        emit error(QString("Ecriture MEMFS impossible: %1").arg(out));
        return false;
    }
    o.close();
    firmwarePath = out;
    return true;
}

// Construit une image flash complete (4 Mo) a partir d'un fichier firmware .bin
// (code desktop conserve tel quel).
bool Esp32Backend::prepareFlashImage(const QString &binPath, QByteArray &img)
{
    if (!QFileInfo::exists(binPath)) {
        emit error(QString("Firmware introuvable: %1").arg(binPath));
        return false;
    }
    const QFileInfo fi(binPath);
    const QString dir = fi.absolutePath();

    img.clear();
    if (isFlashSize(fi.size())) {
        // Image flash complete : lecture directe du contenu
        QFile f(binPath);
        if (!f.open(QIODevice::ReadOnly) || (img = f.readAll()).isEmpty()) {
            emit error(QString("Lecture impossible: %1").arg(binPath));
            return false;
        }
        return true;
    }
    if (fi.size() > FLASH_SIZE_DEFAULT) {
        emit error(QString("Taille firmware invalide (%1 octets).").arg(fi.size()));
        return false;
    }

    img = QByteArray(int(FLASH_SIZE_DEFAULT), char(0xFF));
    if (fi.fileName().endsWith(".factory.bin", Qt::CaseInsensitive)) {
        if (!placeInImage(img, binPath, 0)) return false;
    } else {
        const QString boot = dir + "/bootloader.bin";
        const QString part = dir + "/partitions.bin";
        const QString app0 = findBootApp0(dir);
        const QString manquant = !QFileInfo::exists(boot)  ? QString("bootloader.bin")
                                 : !QFileInfo::exists(part) ? QString("partitions.bin")
                                 : app0.isEmpty()           ? QString("boot_app0.bin")
                                                            : QString();
        if (!manquant.isEmpty()) {
            emit error(QString("Firmware application seul : fichier %1 introuvable (dossier %2). "
                               "Chargez le firmware depuis le dossier de build PlatformIO "
                               "(.pio/build/...) ou fournissez une image flash complete "
                               "/ firmware.factory.bin.")
                           .arg(manquant, dir));
            return false;
        }
        if (!placeInImage(img, boot, OFFSET_BOOTLOADER) ||
            !placeInImage(img, part, OFFSET_PARTITIONS) ||
            !placeInImage(img, app0, OFFSET_BOOT_APP0)  ||
            !placeInImage(img, binPath, OFFSET_APP))
            return false;
    }
    return true;
}

// Reprogramme la flash deja ouverte par QEMU : reecrit le contenu du fichier
// image MEMFS (la flash re-lue par le ROM/bootloader au prochain reset,
// bloc_pread — semantique desktop preserve).
bool Esp32Backend::programFirmware(const QString &binPath)
{
    QByteArray img;
    if (!prepareFlashImage(binPath, img)) return false;

    if (activeFirmwarePath.isEmpty()) {
        // QEMU pas encore init : on se contente de preparer le firmware
        return loadFirmware(binPath);
    }

    QFile f(activeFirmwarePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate) || f.write(img) != img.size()) {
        emit error(QString("Reprogrammation flash impossible: %1").arg(activeFirmwarePath));
        return false;
    }
    f.flush();
    f.close();

    firmwarePath = activeFirmwarePath;
    activeFirmwareStamp = QFileInfo(activeFirmwarePath).lastModified();
    qDebug() << "Esp32Backend: flash reprogrammee:" << activeFirmwarePath;
    return true;
}

// ==================== Construction de argv pour QEMU ====================
void Esp32Backend::buildArgv()
{
    freeArgv();

    activeFirmwarePath = firmwarePath;                    // firmware passe a qemu_init
    activeFirmwareStamp = QFileInfo(firmwarePath).lastModified();

    // pc-bios : les ROMs sont ecrites dans le MEMFS /bios au start() (cf.
    // texte preBios MEMFS plus bas) ; sous wasm applicationDirPath() est un
    // chemin virtuel : le chemin natif desktop ne marche pas.
    QByteArray biosPath = QByteArray("/bios");
    QByteArray driveArg = QString("file=%1,if=mtd,format=raw")
                              .arg(firmwarePath).toUtf8();

    QList<QByteArray> args;
    args << "qemu-system-xtensa"
         << "-M"       << "esp32-picsimlab"
         << "-L"       << biosPath
         << "-nographic"
         << "-serial"  << "null"
         << "-monitor" << "null"
         << "-global"  << "driver=timer.esp32.timg,property=wdt_disable,value=true"
         << "-drive"   << driveArg;

    qemuArgc = args.size();
    qemuArgv = (char**)malloc(sizeof(char*) * (qemuArgc + 1));
    for (int i = 0; i < qemuArgc; ++i) {
        qemuArgv[i] = strdup(args[i].constData());
    }
    qemuArgv[qemuArgc] = nullptr;
}

void Esp32Backend::freeArgv()
{
    if (!qemuArgv) return;
    for (int i = 0; i <= qemuArgc; ++i) {
        if (qemuArgv[i]) free(qemuArgv[i]);
    }
    free(qemuArgv);
    qemuArgv = nullptr;
    qemuArgc = 0;
}

// ==================== Demarrage (init QEMU, une seule fois) ====================
void Esp32Backend::start()
{
    if (running) return;
    if (firmwarePath.isEmpty()) { emit error("Firmware non charge");  return; }

    // ROMs embarquees (ressources wasmroms.qrc) -> MEMFS /bios.
    // QFile::copy marche : lecture qrc, ecriture stdio->MEMFS.
    QDir().mkpath("/bios");
    const QStringList roms = {"esp32-v3-rom.bin", "esp32-v3-rom-app.bin"};
    for (const QString &rom : roms) {
        const QString dst = "/bios/" + rom;
        QFile::remove(dst);                       // reecrit proprement au re-boot
        if (!QFile::copy(":/bios/" + rom, dst) || !QFile::exists(dst)) {
            emit error(QString("Prechargement ROM impossible: %1").arg(dst));
            return;
        }
    }

    static callbacks_t cbs;
    cbs.picsimlab_write_pin     = on_picsimlab_write_pin;
    cbs.picsimlab_dir_pin       = on_picsimlab_dir_pin;
    cbs.picsimlab_i2c_event     = on_picsimlab_i2c_event;
    cbs.picsimlab_spi_event     = on_picsimlab_spi_event;
    cbs.picsimlab_uart_tx_event = on_picsimlab_uart_tx;
    cbs.pinmap                  = devkitc_pinmap;
    cbs.picsimlab_rmt_event     = on_picsimlab_rmt_event;
    qemu_picsimlab_register_callbacks(&cbs);

    buildArgv();

    running = true;
    qemuThread = QThread::create([this]() { this->runQemuThread(); });
    qemuThread->setObjectName("QEMU-ESP32");
    qemuThread->start();

    emit started();
}

void Esp32Backend::runQemuThread()
{
    qDebug() << "Esp32Backend: qemu_init...";
    qemu_init(qemuArgc, qemuArgv);
    m_ready = true;
    QMetaObject::invokeMethod(this, [this]() { emit machineReady(); }, Qt::QueuedConnection);
    EspDiag::mark("qemu_init fini");
    EspDiag::demarrerEchantillonneur();
    qDebug() << "Esp32Backend: qemu_main_loop...";

    // Replique exacte de system/main.c 10.1 dans ce thread, sans exit() :
    //   qemu_init a acquis BQL + replay mutex ; main() appelle bql_unlock +
    //   replay_mutex_unlock puis qemu_default_main qui fait :
    //   replay_mutex_lock / bql_lock / qemu_main_loop / qemu_cleanup /
    //   bql_unlock / replay_mutex_unlock — le statut devient notre retour
    //   de QThread au lieu de exit().
    bql_unlock();            // qemu_init : BQL acquis -> restitue comme main()
    replay_mutex_unlock();
    replay_mutex_lock();
    bql_lock();
    const int status = qemu_main_loop();
    EspDiag::mark("runQemu : main-loop SORTI");
    qDebug() << "Esp32Backend: qemu_cleanup...";
    EspDiag::arreterEchantillonneur();
    qemu_cleanup(status);
    EspDiag::mark("runQemu : cleanup fini");
    bql_unlock();
    replay_mutex_unlock();

    m_ready = false;
    running = false;
    emit stopped();
}

// ==================== Pause / Resume / Reset ====================
//
// pause() utilise qemu_system_vmstop_request_prepare() + request(),
// concues pour etre appelees depuis un thread externe (elles posent un flag
// que la boucle QEMU traite au tour suivant). Semantique desktop conserve.
void Esp32Backend::pause()
{
    if (!running) return;
    EspDiag::mark("pause VM");
    qDebug() << "Esp32Backend: pause VM (vmstop_request)";
    qemu_system_vmstop_request_prepare();
    qemu_system_vmstop_request(RUN_STATE_PAUSED);
}

void Esp32Backend::resume()
{
    if (!running) return;
    EspDiag::mark("resume VM");
    qDebug() << "Esp32Backend: resume VM";
    vm_start();   // vm_start acquiert le BQL lui-meme
}

void Esp32Backend::reset()
{
    if (!running) return;
    EspDiag::mark("reset VM (boot qemu)");
    qDebug() << "Esp32Backend: reset VM (sans BQL, cause GUEST_RESET)";
    // qemu_system_reset_request est thread-safe : il ne faut PAS detenir le
    // BQL pendant l'appel (cf. commentaire desktop).
    qemu_system_reset_request(SHUTDOWN_CAUSE_GUEST_RESET);
}

void Esp32Backend::stop()
{
    // Compatibilite : stop = pause
    pause();
}

// ==================== Arret definitif (fermeture app) ====================
void Esp32Backend::shutdown()
{
    if (!running) return;

    qDebug() << "Esp32Backend: shutdown (arret definitif)";
    EspDiag::mark("backend shutdown : debut");

    qemu_system_shutdown_request(SHUTDOWN_CAUSE_HOST_UI);
    EspDiag::mark("backend : shutdown demande a QEMU");

    if (qemuThread) {
        if (!qemuThread->wait(5000)) {
            EspDiag::mark("backend : wait(5000) TIMEOUT -> terminate");
            qWarning() << "Esp32Backend: QEMU ne repond pas, terminate force";
            qemuThread->terminate();
            qemuThread->wait(1000);
            EspDiag::mark("backend : terminate + wait(1000) fait");
        } else {
            EspDiag::mark("backend : wait(5000) ok, thread QEMU arret");
        }
        delete qemuThread;
        qemuThread = nullptr;
    }
    EspDiag::arreterEchantillonneur();
    running = false;
    EspDiag::mark("backend shutdown : fin");
}

// ==================== Firmware effectivement charge ====================
bool Esp32Backend::firmwareChange() const
{
    if (activeFirmwarePath.isEmpty()) return false;   // rien chargé dans QEMU
    if (firmwarePath != activeFirmwarePath) return true;
    QFileInfo fi(firmwarePath);
    if (!fi.exists()) return false;
    return fi.lastModified() != activeFirmwareStamp;  // meme fichier, modifié (recompilation)
}

// ==================== UI -> QEMU ====================
void Esp32Backend::setDigitalPin(int logicalPin, bool level)
{
    if (!m_ready) return;
    bql_lock();
    qemu_picsimlab_set_pin(logicalPin, level ? 1 : 0);
    bql_unlock();
}

// Tension injectee sur une entree analogique en millivolts (QEMU borne 0..3300)
// Parite desktop : sans BQL explicite (le shim fork gère sa coherence).
void Esp32Backend::setAnalogPinMV(int channel, int mV)
{
    if (m_ready) qemu_picsimlab_set_apin_mv(channel, mV);
}

void Esp32Backend::sendUart(int uartId, const QByteArray &data)
{
    if (m_ready)
        qemu_picsimlab_uart_receive(uartId,
                                    reinterpret_cast<const uint8_t*>(data.constData()),
                                    data.size());
}

// ==================== Introspection ====================
uint32_t *Esp32Backend::getInternal(int cfg)
{
    // Les structures internes n'existent qu'une fois la machine QEMU creee
    return m_ready ? qemu_picsimlab_get_internals(cfg) : nullptr;
}

int Esp32Backend::flashDump(qint64 offset, void *buf, int bytes)
{
    return m_ready ? qemu_picsimlab_flash_dump(offset, buf, bytes) : -1;
}

// ==================== Pont I2C ====================
uint8_t Esp32Backend::handleI2c(uint8_t bus, uint8_t addr, uint16_t event)
{
    if (m_grove) return m_grove->onI2cEvent(bus, addr, event);
    return 0;
}

// ==================== Slots internes (marshalling cross-thread) ====================
void Esp32Backend::emitGpioChanged(int pin, int value)
{
    QMetaObject::invokeMethod(this, [this, pin, value]() {
        emit gpioChanged(pin, value != 0);
    }, Qt::QueuedConnection);
}

void Esp32Backend::emitGpioDir(int pin, int value)
{
    QMetaObject::invokeMethod(this, [this, pin, value]() {
        emit gpioDirectionChanged(pin, value != 0);
    }, Qt::QueuedConnection);
}

void Esp32Backend::emitUartTx(int id, int value)
{
    QMetaObject::invokeMethod(this, [this, id, value]() {
        // POC C M4 : mesure du debit UART livre a l'UI (fenetre de 2 s), tracee
        // uniquement en mode diagnostic (ESPSIMU_DIAG=1) comme le reste d'EspDiag.
        static qint64 s_octets = 0;
        static QElapsedTimer s_epoch;
        if (EspDiag::actif()) {
            if (!s_epoch.isValid()) s_epoch.start();
            ++s_octets;   // un appel = un octet livre a la queue UI
            if (s_epoch.elapsed() >= 2000) {
                const double secs = s_epoch.elapsed() / 1000.0;
                qDebug("M4 debit UART: %lld octets livre en %.1f s (%.1f Ko/s)",
                       (long long)s_octets, secs, s_octets / 1024.0 / secs);
                s_epoch.restart();
                s_octets = 0;
            }
        }
        emit uartTx(id, static_cast<char>(value));
    }, Qt::QueuedConnection);
}

// Signaux de sortie du GPIO matrix : LEDC_HS_SIG_OUT0..7 = 71..78, LEDC_LS_SIG_OUT0..7 = 79..86
#define LEDC_SIG_OUT_FIRST 71
#define LEDC_SIG_OUT_LAST  86

QMap<int,int> Esp32Backend::pwmPins()
{
    QMap<int,int> res;
    uint32_t *out_sel = getInternal(QEMU_INTERNAL_GPIO_OUT_SEL);
    if (!out_sel) return res;
    for (int gpio = 0; gpio < 40; gpio++) {
        uint32_t sel = out_sel[gpio];
        if (sel >= LEDC_SIG_OUT_FIRST && sel <= LEDC_SIG_OUT_LAST)
            res.insert(gpio, int(sel - LEDC_SIG_OUT_FIRST));
    }
    return res;
}

float Esp32Backend::pwmDuty(int channel)
{
    float *duty = reinterpret_cast<float*>(getInternal(QEMU_INTERNAL_LEDC_CHANNEL_DUTY));
    if (!duty || channel < 0 || channel > 15) return -1.0f;
    return duty[channel];
}

uint32_t Esp32Backend::pwmFreq(int channel)
{
    uint32_t *conf = getInternal(QEMU_INTERNAL_LEDC_CHANNEL_CONF);
    uint32_t *freq = getInternal(QEMU_INTERNAL_LEDC_TIMER_FREQ);
    if (!conf || !freq || channel < 0 || channel > 15) return 0;
    int timer = (conf[channel] & 3) + (channel >= 8 ? 4 : 0);
    return freq[timer];
}
