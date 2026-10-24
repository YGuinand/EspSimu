#ifndef LCDSCREEN_H
#define LCDSCREEN_H

#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>

#include <QObject>
#include <cstdint>

// Afficheur HD44780 2x16 (JHD1313M1 du module Grove LCD RGB) : decode les commandes
// et donnees recues en I2C (octets complets, deja assembles) et met a jour le texte affiche.
class LcdScreen : public QObject
{
    Q_OBJECT

public:
    LcdScreen();

private:
    int ascii[16][2];

    int adresse;
    bool allume;
    int contraste;

    void clearDisplay();
    void modif(int x, int y, int c);

public:
    void powerOn();
    void powerOff();

    int getAscii(int x, int y) {  return ascii[x][y];  }

    // Injecte un octet de commande (isData=false) ou de donnee (isData=true)
    void injectI2cByte(bool isData, uint8_t value);

public slots :
    void setContraste(int c);

signals :
    void modifie(int x, int y, int c);
    void setOpacity(double cd);
    void setBacklightColor(int r, int g, int b, int alpha);
};

class LcdScreenView : public QObject
{
    Q_OBJECT

private :
    LcdScreen *screen;

    QGraphicsScene gs;
    QPixmap *font[256];
    QGraphicsPixmapItem tab[16][2];

    void calcFont();

public :
    LcdScreenView(LcdScreen &s);
    ~LcdScreenView();
    QGraphicsScene *getScene() { return &gs; }

public slots :
    void setOpacity(double cd);
    void modifie(int x, int y, int c);
    void setBacklightColor(int r, int g, int b, int alpha);
};

#endif // LCDSCREEN_H
