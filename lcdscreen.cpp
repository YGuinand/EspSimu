#include <stdlib.h>
#include "lcdscreen.h"
#include "lcdfont.h"

#include <QPainter>
#include <QGraphicsView>

LcdScreen::LcdScreen()
{
    allume = false;
    contraste = 128;
    adresse = 0;

    for (int i=0; i<16; i++) {
        ascii[i][0] = 32;
        ascii[i][1] = 32;
    }
}

void LcdScreen::setContraste(int c)
{
    contraste = c & 255;
    double cd = allume ? contraste/255.0 : 0.0;
    emit setOpacity(cd);
}

void LcdScreen::modif(int x, int y, int c)
{
    if ((x>=0)&&(x<16)&&(y>=0)&&(y<2)) {
        if (ascii[x][y] != c) {
            ascii[x][y] = c;
            emit modifie(x, y, c);
        }
    }
}

void LcdScreen::powerOff()
{
    allume = false;
    adresse = 0;
    setContraste(contraste);
    for (int x=0; x<16; x++) {
        modif(x, 0, 32);
        modif(x, 1, 32);
    }
    emit setBacklightColor(255,255,255,0);
}

void LcdScreen::powerOn()
{
    allume = true;
    setContraste(contraste);
    for (int x=0; x<16; x++) {
        modif(x, 1, 32);
        modif(x, 0, 255);
    }
    emit setBacklightColor(255,255,255,255);
}

void LcdScreen::clearDisplay()
{
    for (int x=0; x<16; x++) {
        modif(x, 0, 32);
        modif(x, 1, 32);
    }
}

void LcdScreen::injectI2cByte(bool isData, uint8_t value)
{
    if (!allume) return;

    int val = value;
    if (isData) {
        int x = adresse % 0x40;
        int y = adresse / 0x40;
        modif(x, y, val);
        adresse = (adresse + 1) % 0x80;
    } else {
        if (val == 1) {  clearDisplay();  adresse=0;  } // clear display
        else if ((val&0xFE)==0x02) {  adresse=0;  } // goto home
        else if ((val&0xFC)==0x04) {  } // cursor direction -- not implemented
        else if ((val&0xF8)==0x08) {  } // display - cursor - blink -- not implemented
        else if ((val&0xF0)==0x10) {  // shifts display -- only cursor move implemented
            if ((val&0x08)==0) {
                if ((val&0x04)!=0) adresse++; else adresse--;
            }
        } else if ((val&0xE0)==0x20) {  } // interface data lenght -- not implemented
        else if ((val&0xC0)==0x40) {  } // sets CGRAM address -- not implemented
        else if ((val&0x80)==0x80) {  adresse=val&0x7F;  } // sets DDRAM address
    }
}

LcdScreenView::LcdScreenView(LcdScreen &s) : gs(0,0,288,64)
{
    int i;

    screen = &s;

    for (i=0; i<256; i++) {
        font[i] = new QPixmap(18,32);
    }

    calcFont();

    for (int i=0; i<16; i++) {
        tab[i][0].setPixmap(*font[screen->getAscii(i, 0)]);
        tab[i][0].setPos(i*18,0);
        tab[i][0].setScale(1);
        tab[i][0].setOpacity(0);
        gs.addItem(&tab[i][0]);
        tab[i][1].setPixmap(*font[screen->getAscii(i, 1)]);
        tab[i][1].setPos(i*18,32);
        tab[i][1].setScale(1);
        tab[i][1].setOpacity(0);
        gs.addItem(&tab[i][1]);
    }

    connect(screen, SIGNAL(modifie(int,int,int)), this, SLOT(modifie(int,int,int)));
    connect(screen, SIGNAL(setOpacity(double)), this, SLOT(setOpacity(double)));
    connect(screen, SIGNAL(setBacklightColor(int,int,int,int)), this, SLOT(setBacklightColor(int,int,int,int)));
}

LcdScreenView::~LcdScreenView()
{
    for (int i=0; i<256; i++) {
        delete font[i];
    }
}

void LcdScreenView::calcFont()
{
    QPainter paint;

    for (int i=0; i<256; i++) {
        font[i]->fill(Qt::transparent);
        paint.begin(font[i]);
        for (int y=0; y<7; y++) {
            for (int x=0; x<5; x++) {
                if (lcdFont[i][y][x] != ' ') {
                    paint.setOpacity(1.0);
                } else {
                    paint.setOpacity(0.15);
                }
                paint.setPen(QColor(0,0,0));
                paint.drawPoint(3*x,4*y);
                paint.drawPoint(3*x,4*y+1);
                paint.drawPoint(3*x,4*y+2);
                paint.drawPoint(3*x+1,4*y);
                paint.drawPoint(3*x+1,4*y+1);
                paint.drawPoint(3*x+1,4*y+2);
                paint.setPen(QColor(64,64,64));
                paint.drawPoint(3*x+2,4*y);
                paint.drawPoint(3*x+2,4*y+1);
                paint.drawPoint(3*x+2,4*y+2);
                paint.drawPoint(3*x,4*y+3);
                paint.drawPoint(3*x+1,4*y+3);
                paint.drawPoint(3*x+2,4*y+3);
            }
        }
        paint.end();
    }
}

void LcdScreenView::setOpacity(double cd)
{
    for (int x=0; x<16; x++) {
        for (int y=0; y<2; y++) {
            tab[x][y].setOpacity(cd);
        }
    }
}

void LcdScreenView::modifie(int x, int y, int c)
{
    tab[x][y].setPixmap(*font[c]);
}

void LcdScreenView::setBacklightColor(int r, int g, int b, int alpha)
{
    // Applique la couleur au fond de la scene
    gs.setBackgroundBrush(QBrush(QColor(r, g, b, alpha)));

    // S'assurer que la scene couvre toute la zone du LCD
    gs.setSceneRect(0, 0, 288, 64);
}

