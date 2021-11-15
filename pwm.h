#ifndef PWM_H
#define PWM_H

#include "general.h"
#include "evenement.h"
#include "ports.h"

class P18f4550;

// Modules ccp ne prenant en charge que le mode pwm de base
class Pwm : public Evenement
{
protected :
    P18f4550 *processeur;
    int dutyMemo, dutyLatch;
    bool pwmOn, out;
    int regConfig, regDutyL, regDutyH;
    Port *port;
    Byte pin;
public :
    Pwm(P18f4550 *p, int rc, int rdl, int rdh, Port *prt, Byte n);
    void config();
    void dutyCycle();
    bool isOn() {  return pwmOn;  }
    bool output() {  return out;  }
    void basculeSet();
    virtual void executer();
};

#endif // PWM_H
