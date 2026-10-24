#ifndef ESP32BACKEND_H
#define ESP32BACKEND_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QMap>
#include <cstdint>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#endif

class GroveLcdRgb;

// Brochage de la carte (cf. docs/schema.pdf), en numeros de GPIO ESP32.
// Les API QEMU (setDigitalPin, signal gpioChanged) utilisent le numero
// "logique" = GPIO + 1 (cf. pinmap dans esp32backend.cpp) : utiliser pinLogique().
namespace Esp32Pins {
    constexpr int pinLogique(int gpio) { return gpio + 1; }

    constexpr int LED_VERTE  = 4;    // D1 (net "LED")
    constexpr int LED_JAUNE  = 2;    // D2 (net "LED2")
    constexpr int BP0        = 14;
    constexpr int BP1        = 12;
    constexpr int BP2        = 15;

    // Connecteur J2 (HE10 2x08) : entrees tout ou rien utilisables pour la fiche jack
    // et le fin de course. J2-1 (3,3 V), J2-2 (GND) et J2-15/16 (PWM1/PWM2, hacheurs)
    // sont exclus.
    struct EntreeJ2 {
        int broche;          // numero de broche sur J2
        const char *signal;  // nom du signal sur le schema
        int gpio;            // GPIO ESP32 correspondant
        int adc;             // canal ADC1 (index pour setAnalogPinMV, tension injectee en mV), -1 si pas sur ADC1
    };
    // adc : canal ADC1 (CH0=IO36, CH3=IO39, CH4=IO32, CH5=IO33, CH6=IO34, CH7=IO35)
    constexpr EntreeJ2 ENTREES_J2[] = {
        {  3, "AN2",   33,  5 },
        {  4, "AN1",   32,  4 },
        {  5, "FDC",   16, -1 },
        {  6, "AN3",   35,  7 },
        {  7, "CODA1", 17, -1 },
        {  8, "CODB1", 18, -1 },
        {  9, "CODA2", 19, -1 },
        { 10, "AN4",   34,  6 },
        { 11, "AN6",   39,  3 },
        { 12, "AN5",   36,  0 },
        { 13, "CODB2", 13, -1 },
        { 14, "JACK",  23, -1 },
    };
    constexpr int NB_ENTREES_J2 = sizeof(ENTREES_J2) / sizeof(ENTREES_J2[0]);
}

class Esp32Backend : public QObject
{
    Q_OBJECT
public:
    explicit Esp32Backend(QObject *parent = nullptr);
    ~Esp32Backend();

    // ==================== Cycle de vie ====================
    bool loadLibrary(const QString &libPath);
    bool loadFirmware(const QString &binPath);
    bool programFirmware(const QString &binPath);  // reprogramme la flash deja ouverte par QEMU
    void start();       // Init QEMU + lance le thread (une seule fois)
    void pause();       // Gele la VM (thread reste vivant)
    void resume();      // Reprend la VM
    void reset();       // Reset materiel de la VM
    void shutdown();    // Arret definitif (fermeture app)
    void stop();        // Alias de pause() pour compat

    // ==================== UI -> QEMU ====================
    void setDigitalPin(int logicalPin, bool level);
    void setAnalogPinMV(int channel, int mV);  // entree analogique en millivolts (0..3300, borne par QEMU)
    void sendUart(int uartId, const QByteArray &data);

    // ==================== Introspection QEMU ====================
    uint32_t *getInternal(int cfg);
    int flashDump(qint64 offset, void *buf, int bytes);

    // ==================== Pont I2C ====================
    void setGrove(GroveLcdRgb *g) { m_grove = g; }
    uint8_t handleI2c(uint8_t bus, uint8_t addr, uint16_t event);

    // ==================== Methodes appelees par les callbacks C ====================
    void emitGpioChanged(int pin, int value);
    void emitGpioDir(int pin, int value);
    void emitUartTx(int id, int value);

    bool isRunning() const { return running; }
    bool isMachineReady() const { return m_ready; }

    // true si le firmware prepare (firmwarePath) differe de celui effectivement
    // charge dans QEMU (chemin different ou fichier modifie depuis qemu_init) :
    // il faut alors reprogrammer la flash (programFirmware) avant le prochain reset.
    bool firmwareChange() const;

    // GPIO -> canal LEDC (0..15) pour toutes les broches routees vers un signal LEDC
    QMap<int,int> pwmPins();
    // Duty (0..100 %) d'un canal LEDC, -1 si indisponible
    float pwmDuty(int channel);
    // Frequence PWM (Hz) du timer associe a un canal LEDC, 0 si indisponible
    uint32_t pwmFreq(int channel);

signals:
    void gpioChanged(int logicalPin, bool level);
    void gpioDirectionChanged(int logicalPin, bool output);
    void uartTx(int uartId, char byte);
    void i2cEvent(int id, int addr, int event);
    void spiEvent(int id, int event);
    void rmtEvent(int channel, quint32 config0, quint32 value);
    void started();
    void machineReady();   // machine QEMU creee : les broches peuvent etre pilotees
    void stopped();
    void error(const QString &message);

private slots:
    void runQemuThread();

private:
    void *libHandle;
    QThread *qemuThread;
    std::atomic<bool> running;
    std::atomic<bool> m_ready;
    QString firmwarePath;          // firmware prepare (image flash complete)
    QString activeFirmwarePath;    // firmware effectivement donne a qemu_init
    QDateTime activeFirmwareStamp; // empreinte du fichier activeFirmwarePath
    char **qemuArgv;
    int qemuArgc;

    GroveLcdRgb *m_grove;

    // ---- Fonctions QEMU resolues par dlsym ----
    void (*p_register_callbacks)(void*);
    void (*p_set_pin)(int, int);
    void (*p_set_apin)(int, int);
    void (*p_uart_receive)(int, const uint8_t*, int);
    uint32_t* (*p_get_internals)(int);
    int (*p_flash_dump)(int64_t, void*, int);
    void (*p_qemu_init)(int, char**);
    void (*p_qemu_main_loop)(void);
    void (*p_qemu_cleanup)(void);
    void (*p_system_reset)(int);
    void (*p_system_shutdown)(int);

    // ---- BQL (Big QEMU Lock) ----
    void (*p_bql_lock)(const char*, int);
    void (*p_bql_unlock)(void);

    // ---- Pause / Resume ----
    void (*p_vm_start)(void);
    void (*p_vmstop_prepare)(void);
    void (*p_vmstop_request)(int);

    bool placeInImage(QByteArray &img, const QString &file, qint64 offset);
    bool prepareFlashImage(const QString &binPath, QByteArray &img);
    bool resolveSymbols();
    void buildArgv();
    void freeArgv();
};

#endif // ESP32BACKEND_H
