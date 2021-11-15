#ifndef TIMER0_H
#define TIMER0_H

#include "general.h"
#include "evenement.h"
#include "registre.h"

class P18f4550;
class Timer0;

class RegistreTMR0L : public RegistreSpecial {
protected :
    Timer0* timer0;
public :
    RegistreTMR0L(Timer0 *t) : RegistreSpecial("TMR0L") { timer0 = t; }
    virtual void write(Byte d);
    virtual void setBit(Byte num);
    virtual void clrBit(Byte num);
    virtual Byte read();
//    virtual void powerOnReset() {  write(0);  }
};

class RegistreTMR0H : public RegistreSpecial {
public :
    RegistreTMR0H() : RegistreSpecial("TMR0H") {}
};

class RegistreT0CON : public RegistreSpecial {
protected :
    Timer0* timer0;
public :
    RegistreT0CON(Timer0 *t) : RegistreSpecial("T0CON") { timer0 = t; }
    virtual void write(Byte d);
    virtual void setBit(Byte num);
    virtual void clrBit(Byte num);
//    virtual Byte read() { return data; }
    virtual void powerOnReset() {  write(0xFF);  }
};

class Timer0 : public Evenement
{
protected:
    P18f4550 *processeur;
    bool timerOn, tmr0Output, tmr8bits, extClock, risingEdge, memoExtClock;
    int16_t prescaler; //limite
    int16_t preCount; // valeur
    int32_t tmr0; // comptage sur 8 ou 16 bits

    RegistreTMR0L *regTMR0L;
    RegistreTMR0H *regTMR0H;
    RegistreT0CON *regT0CON;

    void compte();
    void calcEvent();

public:
    Timer0(P18f4550 *p);
    virtual ~Timer0() {}

    void config();
    int getTmr0() {  return tmr0;  }  // renvoie TMR0 sur 16 bits
    virtual void executer();
    void setTmr0(Byte d) {  tmr0 = d;  if (!tmr8bits) tmr0 += (regTMR0H->readDirect()<<8);  preCount = 0;  if (estEnregistre()) calcEvent();  }
    Byte readTmr0() {  if (!tmr8bits) regTMR0H->writeDirect(tmr0>>8);  return tmr0;  }
    void extClockChange(bool val);
};

inline void RegistreT0CON::write(Byte d) {  RegistreSpecial::write(d); timer0->config();  }
inline void RegistreT0CON::setBit(Byte num) {  RegistreSpecial::setBit(num); timer0->config();  }
inline void RegistreT0CON::clrBit(Byte num) {  RegistreSpecial::clrBit(num); timer0->config();  }

inline void RegistreTMR0L::write(Byte d) {  timer0->setTmr0(d);  }
inline void RegistreTMR0L::setBit(Byte num) {  timer0->setTmr0(timer0->getTmr0() | maskSet[num]);  }
inline void RegistreTMR0L::clrBit(Byte num) {   timer0->setTmr0(timer0->getTmr0() & maskClr[num]);  }
inline Byte RegistreTMR0L::read() {  return timer0->readTmr0();  }

#endif // TIMER0_H
