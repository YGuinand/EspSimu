#include "timer2.h"
#include "processeur.h"

Timer2::Timer2(P18f4550 *p)
{
    processeur = p;
    tmr2Output = false;
    // creation des registres et mappage dans DataMemory
    regT2CON = new RegistreT2CON(this);
    processeur->getDM()->modifie(DataMemory::T2CON, regT2CON);
    regTMR2 = new RegistreTMR2(this);
    processeur->getDM()->modifie(DataMemory::TMR2, regTMR2);
    regPR2 = new RegistrePR2(this);
    processeur->getDM()->modifie(DataMemory::PR2, regPR2);
}

void Timer2::config()
{
    if (estEnregistre()) {
        actualiser();
        supprimer();
    }

    Byte t2con = regT2CON->read();
    int cfgPre = t2con & 0x03;
    if (cfgPre==0) prescaler=1; else if (cfgPre==1) prescaler=4; else prescaler=16;
    postscaler = ((t2con & 0x78) >> 3) + 1;
    timerOn = (t2con & 0x4) != 0;
    postCounter = 0;
    tmr2Output = false;
    tmr2 &= 0x3FC0; // remise à zéro du prescaler, tmr2 est sur 14 bits

    if (timerOn) calcEvent();
}

void Timer2::actualiser()
{
    if (!estEnregistre()) return;  // rien à faire
    Word nbClk = tempsEcoule();
    tmr2 = (nbClk * (16 / prescaler));
    Word tmr2sur8b = tmr2 >> 6;   // attention tmr2 est sur 14 bits au lieu de 8 !
    if (tmr2sur8b == pr2 + 1) {
        tmr2 = 0;
        tmr2Output = true;
        processeur->getPwm1()->basculeSet();
        processeur->getPwm2()->basculeSet();
        postCounter++;
        if (postCounter >= postscaler) {
            postCounter = 0;
            processeur->getDM()->setBit(DataMemory::PIR1, 1);  // set TMR2IF
        }
        calcEvent();
    } else {
        tmr2Output = false;
    }
}

void Timer2::executer()
{
    tmr2 = 0;
    tmr2Output = true;
    processeur->getPwm1()->basculeSet();
    processeur->getPwm2()->basculeSet();
    postCounter++;
    if (postCounter >= postscaler) {
        postCounter = 0;
        processeur->getDM()->setBit(DataMemory::PIR1, 1);  // set TMR2IF
    }
    calcEvent();
}

void Timer2::calcEvent()
{
    int nbClk = ((pr2+1) << 6) - tmr2;
    if (nbClk<0) nbClk = 0x4000 - tmr2;
    nbClk = (nbClk * prescaler) / 16;
    if (estEnregistre()) supprimer();
    enregistrer(nbClk, processeur->getListeEvts());
}
