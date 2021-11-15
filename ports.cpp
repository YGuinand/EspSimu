#include <stdlib.h>
#include <math.h>

#include "ports.h"
#include "processeur.h"

const QString Connexion::NCString("NC");

Port::Port(P18f4550 *p)
{
    processeur = p;
    for (int i=0; i<8; i++) { cnx[i] = NULL; analogPin[i] = false; change[i] = true;  }
    analogConfig();
    globalChange = true;
}

DataMemory *Port::getDM()
{
    return processeur->getDM();
}

bool Port::getTTL(double v)
{
    bool ttl;
    bool hasard = true;
    int n;

    if (v<0.8) {
        ttl = false;
        hasard = false;
    } else if (v>3) {
        ttl = true;
        hasard = false;
    }
    if (hasard) {
        n = rand() % 2;
        ttl = n != 0;
    }
    return ttl;
}

void Port::setBit(Byte &val, Byte n, bool s)
{
    if (s) val |= MASK(n); else val &= ~MASK(n);
}

double Port::setCnxValue(Byte n, Byte reg, bool input)
{
    double v, z, vcnx, v0=0.0, zpic;

    if (n<8) {
        if (cnx[n]==NULL) return v0;
        if (input) {
            v = 2.5;
            zpic = 1e9;
        } else {
            v = (reg & MASK(n)) != 0 ? 5 : 0;
            zpic = 1;
        }
        z = cnx[n]->getZ();
        vcnx = cnx[n]->getValue(v);
        v0 = (z*v + zpic*vcnx)/(zpic+z);
        cnx[n]->setValue(v0);
    }
    return v0;
}

double Port::getAnalogValue(Byte pin, double vpic, double tSample)
{
    if (pin>=8) return vpic;
    if (cnx[pin]==NULL) return vpic;

    cnx[pin]->miseAJour();

    double tau=(3000+cnx[pin]->getZ())*25e-12;
    double t = tSample/48e6;
    double vsrc = cnx[pin]->getVin();

    return (vsrc-vpic)*exp(-t/tau) + vsrc;
}

void PortA::evalue()
{
    Byte trisa = getDM()->read(getDM()->TRISA);
    Byte porta = getDM()->read(getDM()->PORTA);
    Byte lata = getDM()->read(getDM()->LATA);
    bool ttl;
    double v;

    // RA0
    if (change[0]) {
        v = setCnxValue(0, lata, (trisa & MASK0)!=0);
        ttl = (analogPin[0]) ? 0 : getTTL(v);
        setBit(porta, 0, ttl);
        change[0] = false;
    }
    // RA1
    if (change[1]) {
        v = setCnxValue(1, lata, (trisa & MASK1)!=0);
        ttl = (analogPin[1]) ? 0 : getTTL(v);
        setBit(porta, 1, ttl);
        change[1] = false;
    }
    // RA2
    if (change[2]) {
        v = setCnxValue(2, lata, (trisa & MASK2)!=0);
        ttl = (analogPin[2]) ? 0 : getTTL(v);
        setBit(porta, 2, ttl);
        change[2] = false;
    }
    // RA3
    if (change[3]) {
        v = setCnxValue(3, lata, (trisa & MASK3)!=0);
        ttl = (analogPin[3]) ? 0 : getTTL(v);
        setBit(porta, 3, ttl);
        change[3] = false;
    }
    // RA4
    if (change[4]) {
        v = setCnxValue(4, lata, (trisa & MASK4)!=0);
        ttl = getTTL(v);
        processeur->getTimer0()->extClockChange(ttl);
        setBit(porta, 4, ttl);
        change[4] = false;
    }
    // RA5
    if (change[5]) {
        v = setCnxValue(5, lata, (trisa & MASK5)!=0);
        ttl = (analogPin[5]) ? 0 : getTTL(v);
        setBit(porta, 5, ttl);
        change[5] = false;
    }
    // RA6
    if (change[6]) {
        v = setCnxValue(6, lata, (trisa & MASK6)!=0);
        ttl = getTTL(v);
        setBit(porta, 6, ttl);
        change[6] = false;
    }
    getDM()->writePortADirect(porta);
}

void PortA::analogConfig()
{
    Byte adcon1L = getDM()->read(getDM()->ADCON1) & 0x0F;
    analogPin[0] = adcon1L < 15;
    analogPin[1] = adcon1L < 14;
    analogPin[2] = adcon1L < 13;
    analogPin[3] = adcon1L < 12;
    analogPin[5] = adcon1L < 11;
    changement(0);
    changement(1);
    changement(2);
    changement(3);
    changement(5);
}

void PortB::evalue()
{
    Byte trisb = getDM()->read(getDM()->TRISB);
    Byte portb = getDM()->read(getDM()->PORTB);
    Byte latb = getDM()->read(getDM()->LATB);
    bool ttl;
    double v;

    // RB0
    if (change[0]) {
        v = setCnxValue(0, latb, (trisb & MASK0)!=0);
        ttl = (analogPin[0]) ? 0 : getTTL(v);
        setBit(portb, 0, ttl);
        change[0] = false;
    }
    // RB1
    if (change[1]) {
        v = setCnxValue(1, latb, (trisb & MASK1)!=0);
        ttl = (analogPin[1]) ? 0 : getTTL(v);
        setBit(portb, 1, ttl);
        change[1] = false;
    }
    // RB2
    if (change[2]) {
        v = setCnxValue(2, latb, (trisb & MASK2)!=0);
        ttl = (analogPin[2]) ? 0 : getTTL(v);
        setBit(portb, 2, ttl);
        change[2] = false;
    }
    // RB3
    if (change[3]) {
        v = setCnxValue(3, latb, (trisb & MASK3)!=0);
        ttl = (analogPin[3]) ? 0 : getTTL(v);
        setBit(portb, 3, ttl);
        change[3] = false;
    }
    // RB4
    if (change[4]) {
        v = setCnxValue(4, latb, (trisb & MASK4)!=0);
        ttl = (analogPin[4]) ? 0 : getTTL(v);
        setBit(portb, 4, ttl);
        change[4] = false;
    }
    // RB5
    if (change[5]) {
        v = setCnxValue(5, latb, (trisb & MASK5)!=0);
        ttl = getTTL(v);
        setBit(portb, 5, ttl);
        change[5] = false;
    }

    getDM()->writePortBDirect(portb);
}

void PortB::analogConfig()
{
    Byte adcon1L = getDM()->read(getDM()->ADCON1) & 0x0F;
    analogPin[0] = adcon1L < 3;
    analogPin[1] = adcon1L < 5;
    analogPin[2] = adcon1L < 7;
    analogPin[3] = adcon1L < 6;
    analogPin[4] = adcon1L < 4;
    changement(0);
    changement(1);
    changement(2);
    changement(3);
    changement(4);
}

void PortC::evalue()
{

    Byte trisc = getDM()->read(getDM()->TRISC);
    Byte portc = getDM()->read(getDM()->PORTC);
    Byte latc = getDM()->read(getDM()->LATC);
    bool ttl;
    double v;

    // RC0
    if (change[0]) {
        v = setCnxValue(0, latc, (trisc & MASK0)!=0);
        ttl = getTTL(v);
        processeur->getTimer1()->extClockChange(ttl);
        setBit(portc, 0, ttl);
        change[0] = false;
    }
    // RC1
    if (change[1]) {
        if (processeur->getPwm2()->isOn()) {
            if (processeur->getPwm2()->output()) v = setCnxValue(1, 0x02, (trisc & MASK1)!=0);
            else v = setCnxValue(1, 0, (trisc & MASK1)!=0);
        } else v = setCnxValue(1, latc, (trisc & MASK1)!=0);
        ttl = getTTL(v);
        setBit(portc, 1, ttl);
        change[1] = false;
    }
    // RC2
    if (change[2]) {
        if (processeur->getPwm1()->isOn()) {
            if (processeur->getPwm1()->output()) v = setCnxValue(2, 0x04, (trisc & MASK2)!=0);
            else v = setCnxValue(2, 0, (trisc & MASK2)!=0);
        } else v = setCnxValue(2, latc, (trisc & MASK2)!=0);
        ttl = getTTL(v);
        setBit(portc, 2, ttl);
        change[2] = false;
    }

    getDM()->writePortCDirect(portc);
}

void PortD::evalue()
{
    Byte trisd = getDM()->read(getDM()->TRISD);
    Byte portd = getDM()->read(getDM()->PORTD);
    Byte latd = getDM()->read(getDM()->LATD);
    bool ttl;
    double v;

    // RD0
    if (change[0]) {
        v = setCnxValue(0, latd, (trisd & MASK0)!=0);
        ttl = getTTL(v);
        setBit(portd, 0, ttl);
        change[0] = false;
    }
    // RD1
    if (change[1]) {
        v = setCnxValue(1, latd, (trisd & MASK1)!=0);
        ttl = getTTL(v);
        setBit(portd, 1, ttl);
        change[1] = false;
    }
    // RD2
    if (change[2]) {
        v = setCnxValue(2, latd, (trisd & MASK2)!=0);
        ttl = getTTL(v);
        setBit(portd, 2, ttl);
        change[2] = false;
    }
    // RD3
    if (change[3]) {
        v = setCnxValue(3, latd, (trisd & MASK3)!=0);
        ttl = getTTL(v);
        setBit(portd, 3, ttl);
        change[3] = false;
    }
    // RD4
    if (change[4]) {
        v = setCnxValue(4, latd, (trisd & MASK4)!=0);
        ttl = getTTL(v);
        setBit(portd, 4, ttl);
        change[4] = false;
    }
    // RD5
    if (change[5]) {
        v = setCnxValue(5, latd, (trisd & MASK5)!=0);
        ttl = getTTL(v);
        setBit(portd, 5, ttl);
        change[5] = false;
    }
    // RD6
    if (change[6]) {
        v = setCnxValue(6, latd, (trisd & MASK6)!=0);
        ttl = getTTL(v);
        setBit(portd, 6, ttl);
        change[6] = false;
    }
    // RD7
    if (change[7]) {
        v = setCnxValue(7, latd, (trisd & MASK7)!=0);
        ttl = getTTL(v);
        setBit(portd, 7, ttl);
        change[7] = false;
    }

    getDM()->writePortDDirect(portd);
}

void PortE::analogConfig()
{
    Byte adcon1L = getDM()->read(getDM()->ADCON1) & 0x0F;
    analogPin[0] = adcon1L < 10;
    analogPin[1] = adcon1L < 9;
    analogPin[2] = adcon1L < 8;
    changement(0);
    changement(1);
    changement(2);
}

void PortE::evalue()
{
    Byte trise = getDM()->read(getDM()->TRISE);
    Byte porte = getDM()->read(getDM()->PORTE);
    Byte late = getDM()->read(getDM()->LATE);
    bool ttl;
    double v;

    // RE0
    if (change[0]) {
        v = setCnxValue(0, late, (trise & MASK0)!=0);
        ttl = (analogPin[0]) ? 0 : getTTL(v);
        setBit(porte, 0, ttl);
        change[0] = false;
    }
    // RE1
    if (change[1]) {
        v = setCnxValue(1, late, (trise & MASK1)!=0);
        ttl = (analogPin[1]) ? 0 : getTTL(v);
        setBit(porte, 1, ttl);
        change[1] = false;
    }
    // RE2
    if (change[2]) {
        v = setCnxValue(2, late, (trise & MASK2)!=0);
        ttl = (analogPin[2]) ? 0 : getTTL(v);
        setBit(porte, 2, ttl);
        change[2] = false;
    }

    getDM()->writePortEDirect(porte);
}


