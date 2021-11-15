#include <stdlib.h>
#include "adc.h"
#include "processeur.h"
#include "ports.h"

ModuleADC::ModuleADC(P18f4550 *p)
{
    processeur = p;
    port = NULL;
    channel = -1;
    debutSample = 0;
    etat = EtatOff;
}

void ModuleADC::reset()
{
    Evenement::reset();
    channel = -1;
    etat = EtatOff;
}

void ModuleADC::configPortsPins()
{
    processeur->getPortA()->analogConfig();
    processeur->getPortB()->analogConfig();
    processeur->getPortE()->analogConfig();
}

void ModuleADC::configClock()
{
    int tmp;
    Byte adcon2 = processeur->getDM()->read(processeur->getDM()->ADCON2);

    justRight = (adcon2 & 0x80) != 0;
    tmp = adcon2 & 0x07;
    switch (tmp) {
    case 0 : clkCfg = 2; break;
    case 1 : clkCfg = 8; break;
    case 2 : clkCfg = 32; break;
    case 3 : clkCfg = -1; break; // not implemented
    case 4 : clkCfg = 4; break;
    case 5 : clkCfg = 16; break;
    case 6 : clkCfg = 64; break;
    case 7 : clkCfg = -1; break; // not implemented
    }
    tmp = (adcon2 >> 3) & 0x07;
    switch (tmp) {
    case 0 : acqCfg = 0; break;
    case 1 : acqCfg = 2; break;
    case 2 : acqCfg = 4; break;
    case 3 : acqCfg = 6; break;
    case 4 : acqCfg = 8; break;
    case 5 : acqCfg = 12; break;
    case 6 : acqCfg = 16; break;
    case 7 : acqCfg = 20; break;
    }
}

void ModuleADC::configControl()
{
    Byte adcon0 = processeur->getDM()->read(processeur->getDM()->ADCON0);
    bool adon = (adcon0 & 0x01) != 0;
    int newChannel = (adcon0 >> 2) & 0x0F;
    bool go = (adcon0 & 0x02) != 0;

    if (channel != newChannel) {
        channel = newChannel;
        debutSample = processeur->time();
        switch (channel) {
        case  0 : port=processeur->getPortA();  pin=0;  break;
        case  1 : port=processeur->getPortA();  pin=1;  break;
        case  2 : port=processeur->getPortA();  pin=2;  break;
        case  3 : port=processeur->getPortA();  pin=3;  break;
        case  4 : port=processeur->getPortA();  pin=5;  break;
        case  5 : port=processeur->getPortE();  pin=0;  break;
        case  6 : port=processeur->getPortE();  pin=1;  break;
        case  7 : port=processeur->getPortE();  pin=2;  break;
        case  8 : port=processeur->getPortB();  pin=2;  break;
        case  9 : port=processeur->getPortB();  pin=3;  break;
        case 10 : port=processeur->getPortB();  pin=1;  break;
        case 11 : port=processeur->getPortB();  pin=4;  break;
        case 12 : port=processeur->getPortB();  pin=0;  break;
        case 13 : port=NULL;  pin=0;  break;
        case 14 : port=NULL;  pin=0;  break;
        case 15 : port=NULL;  pin=0;  break;
        }
    }

    if (estEnregistre()) supprimer();
    if (etat==EtatOff) {
        if (adon) etat = EtatStop;
    } else {
        if (!adon) etat = EtatOff;
        else if (etat==EtatStop) {
            if (go) {
                etat = EtatAcq;
                tad = 0;
                enregistrer(clkCfg, processeur->getListeEvts());
            }
        }
    }
}

void ModuleADC::executer()
{
    switch (etat) {
    case EtatAcq :
        tad++;
        if (tad>=acqCfg) {
            tad = 0;
            if (port != NULL) sample=port->getAnalogValue(pin, sample, processeur->time() - debutSample);
            else sample = 5.0 * (rand() % 2048) / 2047.0; // valeur au hasard (potentiel flottant)
            etat = EtatConv;
        }
        supprimer();
        enregistrer(clkCfg, processeur->getListeEvts());
        break;
    case EtatConv :
        tad++;
        if (tad>=12) { // fin de conversion
            short conv = (sample * 1024 / 5);
            if (conv>1023) conv=1023;
            Byte ah, al;
            if (justRight) {
                al = conv & 0xFF;
                ah = (conv >> 8) & 0x03;
            } else {
                al = (conv & 0x03) << 6;
                ah = (conv >> 2) & 0xFF;
            }
            processeur->getDM()->write(processeur->getDM()->ADRESH, ah);
            processeur->getDM()->write(processeur->getDM()->ADRESL, al);
            processeur->getDM()->clrBit(processeur->getDM()->ADCON0, 1);  // clr GO/DONE -> supprime l'evt
            processeur->getDM()->setBit(processeur->getDM()->PIR1, 6);  // set ADIF
            etat = EtatStop;
            // supprimer();
        } else {
            supprimer();
            enregistrer(clkCfg, processeur->getListeEvts());
        }
        break;
    case EtatStop :
        supprimer();
        break;
    case EtatOff :
        supprimer();
        break;
    }
}
