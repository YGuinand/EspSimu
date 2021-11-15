#include <stdlib.h>
#include "lcdscreen.h"
#include "lcdfont.h"

#include <QPainter>
#include <QGraphicsView>

LcdScreen::LcdScreen() : e(true)
{
    displayOn = true;
    contraste = 128;

    for (int i=0; i<16; i++) {
        ascii[i][0] = 32;
        ascii[i][1] = 32;
    }

    displayOn = true;
    etat = EtatOff;
    dirInput = true;
    adresse = 0;
    e_latch = false;
}

void LcdScreen::setContraste(int c)
{
    contraste = c & 255;
    double cd = (etat==EtatOff) ? 0.0 : contraste/255.0;
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
    displayOn = true;
    etat = EtatOff;
    adresse = 0;
    e_latch = false;
    etat=EtatOff;
    setContraste(contraste);
    for (int x=0; x<16; x++) {
        modif(x, 0, 32);
        modif(x, 1, 32);
    }
    dataIn();
}

void LcdScreen::powerOn()
{
    etat=EtatReset;
    setContraste(contraste);
    for (int x=0; x<16; x++) {
        modif(x, 1, 32);
        modif(x, 0, 255);
    }
    time=0;
    dataIn();
}

void LcdScreen::dataIn()
{
    if (!dirInput) {
        d4.setInput();
        d5.setInput();
        d6.setInput();
        d7.setInput();
        dirInput = true;
    }
}

void LcdScreen::dataOut()
{
    if (dirInput) {
        d4.setOutput();
        d5.setOutput();
        d6.setOutput();
        d7.setOutput();
        dirInput = false;
    }
}

bool LcdScreen::busy()
{
    if (etat == EtatWait) {
        if (time<timeOut) return true;
        etat = EtatInterface4bits;
    }
    return false;
}

void LcdScreen::setWait(unsigned long t) // t en cycles à 250kHz
{
    time = 0;
    etat = EtatWait;
    timeOut = t * 48;
}

void LcdScreen::clock()
{
    if (etat == EtatOff) return;
    time++;
/*
    (*dbg) << time << "e=" << e.b01() <<
            " rs=" << rs.b01() <<
            " rw=" << rw.b01() <<
            " d7=" << d7.b01() <<
            " d6=" << d6.b01() <<
            " d5=" << d5.b01() <<
            " d4=" << d4.b01() <<
            " PortD=" << e.getPort()->getDM()->read(e.getPort()->getDM()->PORTD) <<
            " LatD=" << e.getPort()->getDM()->read(e.getPort()->getDM()->LATD) <<
            " etat=" << etat <<
            " d=" << d <<
            " rwl=" << rw_latch << " " << rw.bit() <<
            " eb=" << e.bit() <<
            endl;
            ;
*/
    if (!e.bit() && !e_latch) { // e reste à 0
        return;
    } else if (e.bit() && e_latch) { // e reste à 1

    } else if (e.bit() && !e_latch) { // e passe de 0 à 1
        if (rw.bit()) { // lecture
            dataOut();
            if (!rs.bit()) { // set busy bit
                d7.setOutput(busy());
            }
        } else { // ecriture
            dataIn();
        }
    } else if (!e.bit() && e_latch) { // e passe de 1 à 0
        if (!rw_latch) {
            switch (etat) {
            case EtatReset :
                if (time>3500*48) {
                    if (!rs_latch && (d==0x03)) {
                        etat = EtatWait4_1ms;
                        time = 0;
                    }
                }
                break;
            case EtatWait4_1ms :
                if (time>1000*48) {
                    if (!rs_latch && (d==0x03)) {
                        etat = EtatWait100us;
                        time = 0;
                    }
                }
                break;
            case EtatWait100us :
                if (time>25*48) {
                    if (!rs_latch && (d==0x03)) {
                        etat = EtatInit;
                        time = 0;
                    }
                }
                break;
            case EtatInit :
                if (!rs_latch && (d==0x02)) {
                    etat = EtatInterface4bits;
                    time = 0;
                }
                break;
            case EtatInterface4bits :
                dhigh = d;
                rs_memo = rs_latch;
                etat = EtatInterface4bitsLow;
                break;
            case EtatInterface4bitsLow :
                dlow = d;
                if (rs_memo == rs_latch) traitement();
                etat = EtatInterface4bits;
                break;
            default :
                ;
            }
        }
        dataIn();
    }

    e_latch = e.bit();
    if (e_latch) {
        rs_latch = rs.bit();
        rw_latch = rw.bit();
        d = (d7.b01()<<3) + (d6.b01()<<2) + (d5.b01()<<1) + d4.b01();
    }
}

void LcdScreen::clearDisplay()
{
    for (int x=0; x<16; x++) {
        modif(x, 0, 32);
        modif(x, 1, 32);
    }
}

void LcdScreen::traitement()
{
    int val = (dhigh << 4) + dlow;
/*
    char buf[200];
    sprintf(buf, "\t\tLCD DECODE DH=%02X DL=%02X D=%02X %c rs=%d", (int)dhigh, (int)dlow, (int)val, (char)val, (int)rs_memo);
    (*dbg) << buf << endl;
*/
    if (rs_memo) {
        int x, y;
        x = adresse % 0x40;
        y = adresse / 0x40;
        modif(x, y, val);
        adresse = (adresse + 1) % 0x80;
        setWait(9);
    } else {
        if (val == 1) {  clearDisplay();  adresse=0;  setWait(350);  } // clear display
        else if ((val&0xFE)==0x02) {  adresse=0;  setWait(350);  } // goto home
        else if ((val&0xFC)==0x04) {  setWait(9);  } // cursor direction -- not implemented
        else if ((val&0xF8)==0x08) {  // display - cursor -blink -- only display implemented
            displayOn = (val & 0x04) != 0;
            setContraste(contraste);
            setWait(9);
        } else if ((val&0xF0)==0x10) {  // shifts display -- only cursor move implemented
            if ((val&0x08)==0) {
                if ((val&0x04)!=0) adresse++; else adresse--;
            }
            setWait(9);
        } else if ((val&0xE0)==0x20) {  setWait(9);  } // interface data lenght -- not implemented
        else if ((val&0xC0)==0x40) {  setWait(9);  } // sets CGRAM address -- not implemented
        else if ((val&0x80)==0x80) {  adresse=val&0x7F;  setWait(9);  } // sets DDRAM address
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
