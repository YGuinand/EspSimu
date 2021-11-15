#include "timer0.h"
#include "processeur.h"

Timer0::Timer0(P18f4550 *p)
{
    processeur = p;
    // creation des registres et mappage dans DataMemory
    regT0CON = new RegistreT0CON(this);
    processeur->getDM()->modifie(DataMemory::T0CON, regT0CON);
    regTMR0L = new RegistreTMR0L(this);
    processeur->getDM()->modifie(DataMemory::TMR0L, regTMR0L);
    regTMR0H = new RegistreTMR0H();
    processeur->getDM()->modifie(DataMemory::TMR0H, regTMR0H);
}

void Timer0::config()
{
    Byte t0con = regT0CON->read();
    int cfgPre = t0con & 0x07;
    int psa = t0con & 0x08;
    if (psa) prescaler=1; else prescaler=2<<cfgPre;
    preCount = 0;
    timerOn = (t0con & 0x80) != 0;
    tmr8bits =(t0con & 0x40) != 0;
    extClock = (t0con & 0x20) != 0;
    risingEdge = (t0con & 0x10) == 0;
    if (!extClock) prescaler *= 4;  // fosc/4

    if ((!timerOn) || (extClock)) {
        if (estEnregistre()) supprimer();
    } else calcEvent();
}

void Timer0::executer()
{
    tmr0++;
    if (tmr8bits) {
        if (tmr0 > 255) {
            tmr0 = 0;
            processeur->getDM()->setBit(DataMemory::INTCON, 2);
        }
    } else {
        if (tmr0 > 65535) {
            tmr0 = 0;
            processeur->getDM()->setBit(DataMemory::INTCON, 2);
        }
    }
    calcEvent();
}

void Timer0::compte()
{
    preCount++;
    if (preCount >= prescaler) {
        preCount = 0;
        tmr0++;
        if (tmr8bits) {
            if (tmr0 > 255) {
                tmr0 = 0;
                processeur->getDM()->setBit(DataMemory::INTCON, 2);
            }
        } else {
            if (tmr0 > 65535) {
                tmr0 = 0;
                processeur->getDM()->setBit(DataMemory::INTCON, 2);
            }
        }
    }
}

void Timer0::calcEvent()
{
    int32_t nbClk = prescaler - preCount;
    if (estEnregistre()) supprimer();
    enregistrer(nbClk, processeur->getListeEvts());
}

void Timer0::extClockChange(bool val)
{
    if ((extClock) && (timerOn)) {
        if (memoExtClock) {
            if (!val) { // front descendant
                memoExtClock = false;
                if (!risingEdge) compte();
            }
        } else {
            if (val) { // front montant
                memoExtClock = true;
                if (risingEdge) compte();
            }
        }
    } else {
        memoExtClock = val;
    }
}
