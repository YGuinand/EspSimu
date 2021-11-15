#include "pwm.h"
#include "processeur.h"
#include "datamemory.h"
#include "timer2.h"

Pwm::Pwm(P18f4550 *p, int rc, int rdl, int rdh, Port *prt, Byte n)
{
    processeur = p;
    pwmOn = false;
    dutyMemo = 0;
    regConfig = rc;
    regDutyL = rdl;
    regDutyH = rdh;
    port = prt;
    pin = n;
}

void Pwm::config()
{
    Byte ccpxcon = processeur->getDM()->read(regConfig);

    if (pwmOn) {  // le module est actif
        if ((ccpxcon & 0xCC) != 0x0C) {  // désactivation
            pwmOn = false;
            port->changement(pin);
        } else {  // mise à jour du rapport cyclique
            dutyMemo &= 0x3FC;
            dutyMemo += ((ccpxcon & 0x30) >> 4);
        }
    } else { // le module est inactif, activation ?
        if ((ccpxcon & 0xCC) == 0x0C) {  // activation
            pwmOn = true;
            out = false;
            port->changement(pin);
            dutyMemo &= 0x3FC;   // mise à jour du rapport cyclique
            dutyMemo += ((ccpxcon & 0x30) >> 4);
        }
    }
}

void Pwm::dutyCycle()
{
    int duty = processeur->getDM()->read(regDutyL);

    dutyMemo &= 0x03;
    dutyMemo += (duty << 2);
}

void Pwm::executer() // bascule reset
{
    if (!pwmOn) return;
    if (out) {
        out = false;
        port->changement(pin);
    }
    supprimer();
}

void Pwm::basculeSet()
{
    if (!pwmOn) return;
    dutyLatch = dutyMemo;
    // latch CCPRxL dans CCPRxH
    pwmOn = false; //débloque l'écriture dans CCPRxH
    processeur->getDM()->write(regDutyH, dutyLatch>>2);
    pwmOn = true;
    out = (dutyLatch != 0);
    if (estEnregistre()) supprimer();
    if (out) {
        Word nbClk = dutyLatch * processeur->getTimer2()->getPrescaler();
        enregistrer(nbClk, processeur->getListeEvts());
    }
    port->changement(pin);
}
