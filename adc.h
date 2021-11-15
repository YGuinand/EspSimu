#ifndef ADC_H
#define ADC_H

#include "general.h"
#include "evenement.h"

class P18f4550;
class Port;

class ModuleADC : public Evenement
{
protected:
    P18f4550 *processeur;
    bool justRight;
    int channel;
    int acqCfg, clkCfg;
    int tad;
    uint64_t debutSample;
    enum EtatADC {EtatOff, EtatStop, EtatAcq, EtatConv};
    EtatADC etat;
    Port *port;
    Byte pin;
    double sample;
public:
    ModuleADC(P18f4550 *p);
    void configControl(); // ADCON0
    void configPortsPins(); // ADCON1
    void configClock(); // ADCON2
    virtual void executer();
    void reset();
};

#endif // ADC_H
