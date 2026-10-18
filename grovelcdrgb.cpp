#include "grovelcdrgb.h"
#include "lcdscreen.h"

#include <QDebug>
#include <QMetaObject>

// ============== Adresses I2C (cf. rgb_lcd.cpp) ==============
static const uint8_t LCD_ADDR    = 0x3E;
static const uint8_t RGB_ADDR_V4 = 0x62;   // PCA9633
static const uint8_t RGB_ADDR_V5 = 0x30;   // SGM31323

// ============== Préfixes LCD ==============
static const uint8_t LCD_CMD  = 0x80;
static const uint8_t LCD_DATA = 0x40;

// ============== Registres RGB ==============
// V4 (PCA9633)
static const uint8_t V4_MODE1  = 0x00;
static const uint8_t V4_MODE2  = 0x01;
static const uint8_t V4_PWM_B  = 0x02;
static const uint8_t V4_PWM_G  = 0x03;
static const uint8_t V4_PWM_R  = 0x04;
static const uint8_t V4_OUTPUT = 0x08;

// V5 (SGM31323)
static const uint8_t V5_REG_CTRL = 0x00;
static const uint8_t V5_REG_R    = 0x06;
static const uint8_t V5_REG_G    = 0x07;
static const uint8_t V5_REG_B    = 0x08;

// ============== Codes event I2C (protocole picsimlab) ==============
// 0x0100 : START (+ addr)
// 0x0200 : WRITE byte (byte dans event & 0xFF)
// 0x0300 : READ request
// 0x0000 : STOP
static const uint16_t I2C_START  = 0x0100;
static const uint16_t I2C_WRITE  = 0x0200;
static const uint16_t I2C_READ   = 0x0300;
static const uint16_t I2C_STOP   = 0x0000;

GroveLcdRgb::GroveLcdRgb(LcdScreen *lcd, QObject *parent)
    : QObject(parent),
    m_lcd(lcd),
    m_addr(0), m_target(TargetNone),
    m_lcdFirstByte(true), m_lcdPrefix(0),
    m_rgbFirstByte(true), m_rgbReg(0),
    m_r(255), m_g(255), m_b(255)     // Seeed setColorWhite() par defaut
{
}

void GroveLcdRgb::resetTransaction()
{
    m_lcdFirstByte = true;
    m_lcdPrefix    = 0;
    m_rgbFirstByte = true;
    m_rgbReg       = 0;
}

uint8_t GroveLcdRgb::onI2cEvent(uint8_t bus, uint8_t addr, uint16_t event)
{
    Q_UNUSED(bus);

    // Encodage observe dans le fork lcgamboa (via les logs) :
    //   event == 0x0001                   -> START
    //   event == 0x0003                   -> STOP
    //   event == (data << 8) | 0x05       -> WRITE byte (data)
    //   event == (data << 8) | 0x04       -> READ byte (a confirmer)
    uint8_t type = event & 0xFF;

    // ---- START ----
    if (type == 0x01) {
        m_addr = addr & 0xFE;
        if (m_addr == LCD_ADDR) {
            m_target = TargetLcd;
        } else if (m_addr == RGB_ADDR_V4) {
            m_target = TargetRgbV4;
        } else if (m_addr == RGB_ADDR_V5) {
            m_target = TargetRgbV5;
        } else {
            m_target = TargetNone;
        }
        resetTransaction();
        return 0;
    }

    // ---- STOP ----
    if (type == 0x03) {
        m_target = TargetNone;
        resetTransaction();
        return 0;
    }

    // ---- WRITE ----
    if (type == 0x05) {
        uint8_t b = (event >> 8) & 0xFF;
        if (m_target == TargetLcd)        handleLcdByte(b);
        else if (m_target == TargetRgbV4) handleRgbByte(b);
        else if (m_target == TargetRgbV5) handleRgbByte(b);
        return 0;
    }

    // ---- READ (a confirmer) ----
    if (type == 0x04) {
        return 0x00;
    }

    return 0;
}

// ======================= LCD =======================
//
// Sequence envoyee par rgb_lcd.cpp :
//   command(cmd)  ->  {0x80, cmd}
//   write(data)   ->  {0x40, data}
//   createChar()  ->  {0x40, d0, d1, ..., d7}   (9 octets)
//
void GroveLcdRgb::handleLcdByte(uint8_t b)
{
    if (m_lcdFirstByte) {
        if (b == LCD_CMD || b == LCD_DATA) {
            m_lcdPrefix    = b;
            m_lcdFirstByte = false;
        }
        // sinon : on ignore (transaction malformee)
        return;
    }

    // Octet de donnee : injection directe dans LcdScreen
    bool isData = (m_lcdPrefix == LCD_DATA);
    uint8_t value = b;

    // ---- Marshalling vers le thread Qt ----
    // LcdScreen vit dans le thread Qt, mais ce callback arrive
    // depuis le thread QEMU. On doit donc passer par invokeMethod.
    if (m_lcd) {
        QMetaObject::invokeMethod(m_lcd,
                                  [lcd = m_lcd, isData, value]() {
                                      lcd->injectI2cByte(isData, value);
                                  },
                                  Qt::QueuedConnection);
    }

    // Important : NE PAS remettre m_lcdFirstByte a true ici.
    // createChar envoie {0x40, d0..d7} en une seule transaction :
    // les 8 octets suivants sont aussi des donnees.
    // On garde donc m_lcdPrefix = 0x40 jusqu'au prochain START.
}

// ======================= RGB =======================
//
// Sequence envoyee par rgb_lcd.cpp :
//   setReg(reg, dat)  ->  {reg, dat}
//
void GroveLcdRgb::handleRgbByte(uint8_t b)
{
    if (m_rgbFirstByte) {
        m_rgbReg       = b;
        m_rgbFirstByte = false;
        return;
    }

    uint8_t val = b;
    bool isV5 = (m_target == TargetRgbV5);
    m_rgbFirstByte = true;

    updateColor(m_rgbReg, val, isV5);
}

void GroveLcdRgb::updateColor(uint8_t reg, uint8_t val, bool isV5)
{
    bool changed = false;

    if (isV5) {
        switch (reg) {
        case V5_REG_R: if (m_r != val) { m_r = val; changed = true; } break;
        case V5_REG_G: if (m_g != val) { m_g = val; changed = true; } break;
        case V5_REG_B: if (m_b != val) { m_b = val; changed = true; } break;
        default: return;   // autres registres ignores
        }
    } else {
        switch (reg) {
        case V4_PWM_R: if (m_r != val) { m_r = val; changed = true; } break;
        case V4_PWM_G: if (m_g != val) { m_g = val; changed = true; } break;
        case V4_PWM_B: if (m_b != val) { m_b = val; changed = true; } break;
        default: return;   // MODE1, MODE2, OUTPUT ignores
        }
    }

    if (changed) {
        // On emet depuis le thread QEMU : QueuedConnection implicite
        // car on est dans un objet QObject du thread principal.
        // En pratique, l'emission directe est ici suffisante car on
        // n'appelle pas de methode thread-unsafe sur le recepteur ;
        // le slot connecte sera execute dans le thread du recepteur.
        emit backlightChanged(m_r, m_g, m_b, 255);
    }
}

void GroveLcdRgb::resetEtat()
{
    m_target = TargetNone;
    m_addr = 0;
    resetTransaction();
    m_r = 255; m_g = 255; m_b = 255;
}
