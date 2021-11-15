#ifndef LCDSCREEN_H
#define LCDSCREEN_H

#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>

#include "ports.h"

class ConnexionLcd : public Connexion {
private :
    bool etat;
    bool e;
public :
    ConnexionLcd(bool isE=false) {  e=isE;  }
    virtual void modifie(double v);
    bool bit() {  return etat;  }
    int b01() {  return (etat) ? 1 : 0;  }
};

class LcdScreen : public QObject
{
    Q_OBJECT

public:
    LcdScreen();

private:
    int ascii[16][2];

    ConnexionLcd e, rs, rw, d4, d5, d6, d7;
    int d, dhigh, dlow; // latch d4...d7
    bool rw_latch, rs_latch, rs_memo, e_latch;
    int adresse;
    bool displayOn;
    int contraste;
    unsigned long timeOut;

    unsigned long time;
    enum EtatLCD { EtatOff, EtatReset, EtatWait4_1ms, EtatWait100us, EtatInit,
                   EtatInterface4bits, EtatInterface4bitsLow, EtatWait};
    EtatLCD etat;
    bool dirInput;

    void dataIn();
    void dataOut();
    bool busy();
    void setWait(unsigned long t);
    void traitement();
    void clearDisplay();
    void modif(int x, int y, int c);

public:
    void powerOn();
    void powerOff();

    Connexion *getE() { return &e; }
    Connexion *getRS() { return &rs; }
    Connexion *getRW() { return &rw; }
    Connexion *getD4() { return &d4; }
    Connexion *getD5() { return &d5; }
    Connexion *getD6() { return &d6; }
    Connexion *getD7() { return &d7; }
    void clock();
    void reset() { if (etat!=EtatOff) etat=EtatReset; }

    int getAscii(int x, int y) {  return ascii[x][y];  }

public slots :
    void setContraste(int c);

signals :
    void modifie(int x, int y, int c);
    void setOpacity(double cd);
};

inline void ConnexionLcd::modifie(double v) { etat = v > 2.5;  }

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
};

#endif // LCDSCREEN_H
