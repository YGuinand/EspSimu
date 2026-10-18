#ifndef GROVELCDRGB_H
#define GROVELCDRGB_H

#include <QObject>
#include <cstdint>

class LcdScreen;

class GroveLcdRgb : public QObject
{
    Q_OBJECT
public:
    explicit GroveLcdRgb(LcdScreen *lcd, QObject *parent = nullptr);

    // Appelé par Esp32Backend depuis un callback QEMU (thread QEMU).
    // Retourne le byte à envoyer au maître pour les lectures I2C.
    uint8_t onI2cEvent(uint8_t bus, uint8_t addr, uint16_t event);
    void resetEtat();

signals:
    void backlightChanged(int r, int g, int b, int alpha);

private:
    enum Target { TargetNone, TargetLcd, TargetRgbV4, TargetRgbV5 };

    LcdScreen *m_lcd;

    // Adresse courante de la transaction
    uint8_t m_addr;
    Target  m_target;

    // ---- État LCD ----
    // La bibliothèque Seeed envoie {0x80, cmd} ou {0x40, data}
    // ou {0x40, d0..d7} pour createChar (9 octets).
    bool     m_lcdFirstByte;   // true : on attend 0x80 ou 0x40
    uint8_t  m_lcdPrefix;      // 0x80 (cmd) ou 0x40 (data)

    // ---- État RGB ----
    bool     m_rgbFirstByte;   // true : on attend le numéro de registre
    uint8_t  m_rgbReg;         // registre en cours d'écriture

    // ---- Couleurs courantes (0-255) ----
    uint8_t  m_r, m_g, m_b;

    void resetTransaction();
    void handleLcdByte(uint8_t b);
    void handleRgbByte(uint8_t b);
    void updateColor(uint8_t reg, uint8_t val, bool isV5);
};

#endif // GROVELCDRGB_H
