// =====================================================================
// POC A (wasm) — Esp32Backend stub : interface identique a esp32backend.h,
// aucune acces QEMU (dlopen impossible sous wasm). Toutes les operations
// sont des no-op proprement degradees ; loadLibrary() rend toujours false
// comme un echec de chargement de lib, ce que le modele sait deja gerer.
// =====================================================================
#include "esp32backend.h"

Esp32Backend::Esp32Backend(QObject *parent)
    : QObject(parent)
    , libHandle(nullptr)
    , qemuThread(nullptr)
    , running(false)
    , m_ready(false)
    , qemuArgv(nullptr)
    , qemuArgc(0)
    , m_grove(nullptr)
    , p_register_callbacks(nullptr)
    , p_set_pin(nullptr)
    , p_set_apin(nullptr)
    , p_uart_receive(nullptr)
    , p_get_internals(nullptr)
    , p_flash_dump(nullptr)
    , p_qemu_init(nullptr)
    , p_qemu_main_loop(nullptr)
    , p_qemu_cleanup(nullptr)
    , p_system_reset(nullptr)
    , p_system_shutdown(nullptr)
    , p_bql_lock(nullptr)
    , p_bql_unlock(nullptr)
    , p_vm_start(nullptr)
    , p_vmstop_prepare(nullptr)
    , p_vmstop_request(nullptr)
{
}

Esp32Backend::~Esp32Backend()
{
    shutdown();
}

bool Esp32Backend::loadLibrary(const QString &)
{
    return false;   // backend QEMU indisponible en build wasm
}

bool Esp32Backend::loadFirmware(const QString &)
{
    return false;
}

bool Esp32Backend::programFirmware(const QString &)
{
    return false;
}

void Esp32Backend::start()
{
    emit error(tr("Backend ESP32 non disponible dans ce build (wasm)"));
}

void Esp32Backend::pause()
{
}

void Esp32Backend::resume()
{
}

void Esp32Backend::reset()
{
}

void Esp32Backend::shutdown()
{
}

void Esp32Backend::stop()
{
}

void Esp32Backend::setDigitalPin(int, bool)
{
}

void Esp32Backend::setAnalogPinMV(int, int)
{
}

void Esp32Backend::sendUart(int, const QByteArray &)
{
}

uint32_t *Esp32Backend::getInternal(int)
{
    return nullptr;
}

int Esp32Backend::flashDump(qint64, void *, int)
{
    return -1;
}

uint8_t Esp32Backend::handleI2c(uint8_t, uint8_t, uint16_t)
{
    return 0;
}

void Esp32Backend::emitGpioChanged(int, int)
{
}

void Esp32Backend::emitGpioDir(int, int)
{
}

void Esp32Backend::emitUartTx(int, int)
{
}

bool Esp32Backend::firmwareChange() const
{
    return false;
}

QMap<int,int> Esp32Backend::pwmPins()
{
    return {};
}

float Esp32Backend::pwmDuty(int)
{
    return -1.0f;
}

uint32_t Esp32Backend::pwmFreq(int)
{
    return 0;
}

void Esp32Backend::runQemuThread()
{
}

bool Esp32Backend::placeInImage(QByteArray &, const QString &, qint64)
{
    return false;
}

bool Esp32Backend::prepareFlashImage(const QString &, QByteArray &)
{
    return false;
}

bool Esp32Backend::resolveSymbols()
{
    return false;
}

void Esp32Backend::buildArgv()
{
}

void Esp32Backend::freeArgv()
{
}
