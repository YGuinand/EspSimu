#include "timer1.h"
#include "processeur.h"

Timer1::Timer1(P18f4550 *p)
{
    processeur = p;
    // creation des registres et mappage dans DataMemory
    regT1CON = new RegistreT1CON(this);
    processeur->getDM()->modifie(DataMemory::T1CON, regT1CON);
    regTMR1L = new RegistreTMR1L(this);
    processeur->getDM()->modifie(DataMemory::TMR1L, regTMR1L);
    regTMR1H = new RegistreTMR1H(this);
    processeur->getDM()->modifie(DataMemory::TMR1H, regTMR1H);
}

void Timer1::config()
{
    Byte t1con = regT1CON->read();
    timerOn = (t1con & 0x01) != 0;
    extClock = (t1con & 0x02) != 0;
    t1syncN = (t1con & 0x04) != 0; // non implémenté
    t1oscen = (t1con & 0x08) != 0; // non implémenté
    int cfgPre = (t1con & 0x30) >> 4;
    t1run = (t1con & 0x40) != 0; // non implémenté
    rd16 =(t1con & 0x80) != 0;
    regTMR1H->writeDirect(tmr1 >> 8);
    prescaler = 1 << cfgPre;
    preCount = 0;
    if (!extClock) prescaler *= 4;  // fosc/4

    if ((!timerOn) || (extClock)) {
        if (estEnregistre()) supprimer();
    } else calcEvent();
}

void Timer1::executer()
{
    tmr1++;
    if (tmr1 > 65535) {
        tmr1 = 0;
        processeur->getDM()->setBit(DataMemory::PIR1, 0);
    }
    calcEvent();
}

void Timer1::compte()
{
    preCount++;
    if (preCount >= prescaler) {
        preCount = 0;
        tmr1++;
        if (tmr1 > 65535) {
            tmr1 = 0;
            processeur->getDM()->setBit(DataMemory::PIR1, 0);
        }
    }
}

void Timer1::calcEvent()
{
    int32_t nbClk = prescaler - preCount;
    if (estEnregistre()) supprimer();
    enregistrer(nbClk, processeur->getListeEvts());
}

void Timer1::extClockChange(bool val)
{
    if ((extClock) && (timerOn)) {
        if (memoExtClock) {
            if (!val) { // front descendant
                memoExtClock = false;
            }
        } else {
            if (val) { // front montant
                memoExtClock = true;
                compte();
            }
        }
    } else {
        memoExtClock = val;
    }
}

void Timer1::setTmr1(Byte d)
{
    if (rd16) {
        tmr1 = d;
        tmr1 += (regTMR1H->readDirect() << 8);
    } else {
        tmr1 = (tmr1 & 0xFF00) + d;
    }
    preCount = 0;
    if (estEnregistre()) calcEvent();
}

Byte Timer1::readTmr1()
{
    if (rd16) regTMR1H->writeDirect(tmr1 >> 8);
    return tmr1;
}

void Timer1::setTmr1H(Byte d)
{
    if (rd16) {
        regTMR1H->writeDirect(d);
    } else {
        tmr1 = (tmr1 & 0x00FF) + (d << 8);
    }
}

void Timer1::setBitTmr1H(Byte num)
{
    if (rd16) {
        regTMR1H->writeDirect(regTMR1H->readDirect() | Registre::maskSet[num]);
    } else {
        int32_t d = Registre::maskSet[num];
        tmr1 = tmr1 | (d << 8);
    }
}

void Timer1::clrBitTmr1H(Byte num)
{
    if (rd16) {
        regTMR1H->writeDirect(regTMR1H->readDirect() & Registre::maskClr[num]);
    } else {
        int32_t d = Registre::maskClr[num];
        tmr1 = tmr1 & ((d << 8) | 0xFF);
    }
}

Byte Timer1::readTmr1H()
{
    if (rd16) {
        return regTMR1H->readDirect();
    } else {
        return tmr1 >> 8;
    }
}
