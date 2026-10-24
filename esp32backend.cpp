#include "esp32backend.h"
#include "diagnostic.h"
#include "grovelcdrgb.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <cstring>
#include <cstdlib>
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QMetaObject>
#include <QCoreApplication>

#ifdef _WIN32
#define DL_SYM(handle, name) (void *)GetProcAddress((HMODULE)(handle), (name))
#else
#define DL_SYM(handle, name) dlsym((handle), (name))
#endif
#include <cstdlib>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QMetaObject>
#include <QCoreApplication>

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

// ==================== RunState (include/sysemu/runstate.h) ====================
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
    m_grove(nullptr),
    p_register_callbacks(nullptr),
    p_set_pin(nullptr),
    p_set_apin(nullptr),
    p_uart_receive(nullptr),
    p_get_internals(nullptr),
    p_flash_dump(nullptr),
    p_qemu_init(nullptr),
    p_qemu_main_loop(nullptr),
    p_qemu_cleanup(nullptr),
    p_system_reset(nullptr),
    p_system_shutdown(nullptr),
    p_bql_lock(nullptr),
    p_bql_unlock(nullptr),
    p_vm_start(nullptr),
    p_vmstop_prepare(nullptr),
    p_vmstop_request(nullptr)
{
    g_backend = this;
}

Esp32Backend::~Esp32Backend()
{
    shutdown();
    if (firmwarePath.contains("espsimu_flash_") && firmwarePath.startsWith(QDir::tempPath()))
        QFile::remove(firmwarePath);
    if (libHandle) {
#ifdef _WIN32
        FreeLibrary((HMODULE)libHandle);
#else
        dlclose(libHandle);
#endif
        libHandle = nullptr;
    }
    freeArgv();
    if (g_backend == this) g_backend = nullptr;
}

// ==================== Chargement de la bibliotheque ====================
bool Esp32Backend::loadLibrary(const QString &libPath)
{
    if (libHandle) {
        emit error("Bibliotheque deja chargee");
        return false;
    }

#ifdef _WIN32
    libHandle = LoadLibraryA(libPath.toUtf8().constData());
#else
    libHandle = dlopen(libPath.toUtf8().constData(), RTLD_NOW | RTLD_GLOBAL);
#endif
    if (!libHandle) {
#ifdef _WIN32
        emit error(QString("LoadLibrary: %1").arg((quint64)GetLastError()));
#else
        emit error(QString("dlopen: %1").arg(dlerror()));
#endif
        return false;
    }
    EspDiag::init(libHandle);   // diagnostic deja actif/inactif selon ESPSIMU_DIAG
    return resolveSymbols();
}

bool Esp32Backend::resolveSymbols()
{
    // --- Fonctions QEMU principales ---
    p_register_callbacks = (void(*)(void*))               DL_SYM(libHandle, "qemu_picsimlab_register_callbacks");
    p_set_pin            = (void(*)(int,int))             DL_SYM(libHandle, "qemu_picsimlab_set_pin");
    p_set_apin           = (void(*)(int,int))             DL_SYM(libHandle, "qemu_picsimlab_set_apin_mv");
    p_uart_receive       = (void(*)(int,const uint8_t*,int)) DL_SYM(libHandle, "qemu_picsimlab_uart_receive");
    p_get_internals      = (uint32_t*(*)(int))            DL_SYM(libHandle, "qemu_picsimlab_get_internals");
    p_flash_dump         = (int(*)(int64_t,void*,int))    DL_SYM(libHandle, "qemu_picsimlab_flash_dump");
    p_qemu_init          = (void(*)(int,char**))          DL_SYM(libHandle, "qemu_init");
    p_qemu_main_loop     = (void(*)(void))                DL_SYM(libHandle, "qemu_main_loop");
    p_qemu_cleanup       = (void(*)(void))                DL_SYM(libHandle, "qemu_cleanup");
    p_system_reset       = (void(*)(int))                 DL_SYM(libHandle, "qemu_system_reset_request");
    p_system_shutdown    = (void(*)(int))                 DL_SYM(libHandle, "qemu_system_shutdown_request");

    // --- BQL (QEMU 9.x : API moderne) ---
    p_bql_lock   = (void(*)(const char*,int)) DL_SYM(libHandle, "bql_lock_impl");
    p_bql_unlock = (void(*)(void))            DL_SYM(libHandle, "bql_unlock");
    if (!p_bql_lock || !p_bql_unlock) {
        // Fallback ancienne API (QEMU 4.x-8.x)
        p_bql_lock   = (void(*)(const char*,int)) DL_SYM(libHandle, "qemu_mutex_lock_iothread");
        p_bql_unlock = (void(*)(void))            DL_SYM(libHandle, "qemu_mutex_unlock_iothread");
    }

    // --- Pause / Resume ---
    p_vm_start        = (void(*)(void))  DL_SYM(libHandle, "vm_start");
    p_vmstop_prepare  = (void(*)(void))  DL_SYM(libHandle, "qemu_system_vmstop_request_prepare");
    p_vmstop_request  = (void(*)(int))   DL_SYM(libHandle, "qemu_system_vmstop_request");

    // --- Verifications des symboles essentiels ---
    if (!p_register_callbacks) { emit error("Symbole manquant: qemu_picsimlab_register_callbacks"); return false; }
    if (!p_set_pin)            { emit error("Symbole manquant: qemu_picsimlab_set_pin");            return false; }
    if (!p_set_apin)           { emit error("Symbole manquant: qemu_picsimlab_set_apin_mv");         return false; }
    if (!p_uart_receive)       { emit error("Symbole manquant: qemu_picsimlab_uart_receive");       return false; }
    if (!p_get_internals)      { emit error("Symbole manquant: qemu_picsimlab_get_internals");      return false; }
    if (!p_flash_dump)         { emit error("Symbole manquant: qemu_picsimlab_flash_dump");         return false; }
    if (!p_qemu_init)          { emit error("Symbole manquant: qemu_init");                          return false; }
    if (!p_qemu_main_loop)     { emit error("Symbole manquant: qemu_main_loop");                     return false; }
    if (!p_qemu_cleanup)       { emit error("Symbole manquant: qemu_cleanup");                       return false; }

    if (!p_bql_lock || !p_bql_unlock)
        qWarning() << "Esp32Backend: symboles BQL absents";
    if (!p_system_reset)
        qWarning() << "Esp32Backend: qemu_system_reset_request absent";
    if (!p_system_shutdown)
        qWarning() << "Esp32Backend: qemu_system_shutdown_request absent";
    if (!p_vm_start)
        qWarning() << "Esp32Backend: vm_start absent";
    if (!p_vmstop_prepare || !p_vmstop_request)
        qWarning() << "Esp32Backend: vmstop_* absents";

    return true;
}

// ==================== Firmware ====================
// Formats acceptes :
//  - image flash complete (2/4/8/16 Mo) : utilisee telle quelle ;
//  - firmware.factory.bin de PlatformIO : place a l'offset 0 puis complete a 4 Mo ;
//  - firmware.bin de PlatformIO (application seule) : fusionne avec
//    bootloader.bin et partitions.bin (meme dossier) et boot_app0.bin
//    (dossier ou framework PlatformIO) dans une image 4 Mo temporaire.
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
    const QString pioPackages = QDir::homePath() + "/.platformio/packages";
    const QStringList fw = QDir(pioPackages).entryList(QStringList() << "framework-arduinoespressif32*",
                                                      QDir::Dirs);
    for (const QString &d : fw) {
        QString c = pioPackages + "/" + d + "/tools/partitions/boot_app0.bin";
        if (QFileInfo::exists(c)) return c;
    }
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

    const QString out = QDir::temp().filePath(
        QString("espsimu_flash_%1.bin").arg(QCoreApplication::applicationPid()));
    QFile o(out);
    if (!o.open(QIODevice::WriteOnly | QIODevice::Truncate) || o.write(img) != img.size()) {
        emit error(QString("Ecriture impossible: %1").arg(out));
        return false;
    }
    o.close();
    firmwarePath = out;
    return true;
}

// Construit une image flash complete (4 Mo) a partir d'un fichier firmware .bin
// (image flash de la taille attendue, firmware PlatformIO assemble avec
// bootloader.bin + partitions.bin + boot_app0.bin, ou firmware.factory.bin).
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
        if (!QFileInfo::exists(boot) || !QFileInfo::exists(part) || app0.isEmpty()) {
            emit error("Firmware application seul : bootloader.bin, partitions.bin et "
                       "boot_app0.bin sont requis (dossier de build PlatformIO), "
                       "ou fournissez une image flash complete / firmware.factory.bin.");
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

// Programme le firmware dans la flash telle qu'elle est actuellement ouverte par
// QEMU (carte demarree, meme en pause). En effet, QEMU ne re-lit la flash
// (-drive) qu'au qemu_init : pour charger un nouveau firmware sans re-initialiser
// la machine, on reecrit le contenu du fichier image deja ouvert — la lecture de
// la flash par le ROM/bootloader passe par blk_pread (pas de cache interne
// persistant), donc le prochain reset demarrera sur le nouveau programme.
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

    firmwarePath = activeFirmwarePath;   // le firmware prepare = la flash chargee
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

    QByteArray biosPath = (QCoreApplication::applicationDirPath() + "/pc-bios").toUtf8();
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
    if (!libHandle)     { emit error("Bibliotheque non chargee"); return; }
    if (firmwarePath.isEmpty()) { emit error("Firmware non charge");  return; }

#ifdef _WIN32
    _putenv_s("QEMU_DATADIR", "pc-bios");
#else
    setenv("QEMU_DATADIR", "pc-bios", 1);
#endif

    static callbacks_t cbs;
    cbs.picsimlab_write_pin     = on_picsimlab_write_pin;
    cbs.picsimlab_dir_pin       = on_picsimlab_dir_pin;
    cbs.picsimlab_i2c_event     = on_picsimlab_i2c_event;
    cbs.picsimlab_spi_event     = on_picsimlab_spi_event;
    cbs.picsimlab_uart_tx_event = on_picsimlab_uart_tx;
    cbs.pinmap                  = devkitc_pinmap;
    cbs.picsimlab_rmt_event     = on_picsimlab_rmt_event;
    p_register_callbacks(&cbs);

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
    p_qemu_init(qemuArgc, qemuArgv);
    m_ready = true;
    QMetaObject::invokeMethod(this, [this]() { emit machineReady(); }, Qt::QueuedConnection);
    EspDiag::mark("qemu_init fini");
    EspDiag::demarrerEchantillonneur();
    qDebug() << "Esp32Backend: qemu_main_loop...";
    p_qemu_main_loop();
    qDebug() << "Esp32Backend: qemu_cleanup...";
    EspDiag::arreterEchantillonneur();
    p_qemu_cleanup();
    m_ready = false;
    running = false;
    emit stopped();
}

// ==================== Pause / Resume / Reset ====================
//
// pause() utilise qemu_system_vmstop_request_prepare() + request(),
// qui sont concues pour etre appelees depuis un thread externe
// (elles posent un flag que la boucle QEMU traite au tour suivant).
//
void Esp32Backend::pause()
{
    if (!running) return;
    if (p_vmstop_prepare && p_vmstop_request) {
        EspDiag::mark("pause VM");
        qDebug() << "Esp32Backend: pause VM (vmstop_request)";
        p_vmstop_prepare();
        p_vmstop_request(RUN_STATE_PAUSED);
    } else {
        qWarning() << "Esp32Backend: vmstop_* indisponibles, pause ignoree";
    }
}

void Esp32Backend::resume()
{
    if (!running) return;
    if (p_vm_start) {
        EspDiag::mark("resume VM");
        qDebug() << "Esp32Backend: resume VM";
        p_vm_start();   // vm_start acquiert le BQL lui-meme
    } else {
        qWarning() << "Esp32Backend: vm_start indisponible, resume ignore";
    }
}

void Esp32Backend::reset()
{
    if (!running) return;
    if (!p_system_reset) {
        qWarning() << "Esp32Backend: pas de fonction reset";
        return;
    }

    EspDiag::mark("reset VM (boot qemu)");
    qDebug() << "Esp32Backend: reset VM (sans BQL, cause GUEST_RESET)";
    // qemu_system_reset_request est thread-safe (pose un drapeau + notifie la boucle QEMU).
    // Il ne faut PAS detenir le BQL pendant l'appel, sinon le thread QEMU ne peut pas
    // traiter le reset (il est bloque en attente du BQL que ce thread detient).
    p_system_reset(SHUTDOWN_CAUSE_GUEST_RESET);
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

    if (p_system_shutdown) {
        p_system_shutdown(SHUTDOWN_CAUSE_HOST_UI);
    }

    if (qemuThread) {
        if (!qemuThread->wait(5000)) {
            qWarning() << "Esp32Backend: QEMU ne repond pas, terminate force";
            qemuThread->terminate();
            qemuThread->wait(1000);
        }
        delete qemuThread;
        qemuThread = nullptr;
    }
    EspDiag::arreterEchantillonneur();
    running = false;
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
    if (!p_set_pin || !m_ready) return;
    if (p_bql_lock)   p_bql_lock(__FILE__, __LINE__);
    p_set_pin(logicalPin, level ? 1 : 0);
    if (p_bql_unlock) p_bql_unlock();
}

// Tension injectee sur une entree analogique en millivolts (QEMU borne 0..3300)
void Esp32Backend::setAnalogPinMV(int channel, int mV)
{
    if (p_set_apin && m_ready) p_set_apin(channel, mV);
}

void Esp32Backend::sendUart(int uartId, const QByteArray &data)
{
    if (p_uart_receive && m_ready)
        p_uart_receive(uartId,
                       reinterpret_cast<const uint8_t*>(data.constData()),
                       data.size());
}

// ==================== Introspection ====================
uint32_t *Esp32Backend::getInternal(int cfg)
{
    // Les structures internes n'existent qu'une fois la machine QEMU creee
    return (p_get_internals && m_ready) ? p_get_internals(cfg) : nullptr;
}

int Esp32Backend::flashDump(qint64 offset, void *buf, int bytes)
{
    return (p_flash_dump && m_ready) ? p_flash_dump(offset, buf, bytes) : -1;
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
