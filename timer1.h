#ifndef TIMER1_H
#define TIMER1_H

#include "general.h"
#include "evenement.h"
#include "registre.h"

class P18f4550;
class Timer1;

class RegistreTMR1L : public RegistreSpecial {
protected :
    Timer1* timer1;
public :
    RegistreTMR1L(Timer1 *t) : RegistreSpecial("TMR1L") { timer1 = t; }
    virtual void write(Byte d);
    virtual void setBit(Byte num);
    virtual void clrBit(Byte num);
    virtual Byte read();
//    virtual void powerOnReset() {  write(0);  }
};

class RegistreTMR1H : public RegistreSpecial {
protected :
    Timer1* timer1;
public :
    RegistreTMR1H(Timer1 *t) : RegistreSpecial("TMR1H") { timer1 = t; }
    virtual void write(Byte d);
    virtual void setBit(Byte num);
    virtual void clrBit(Byte num);
    virtual Byte read();
    //    virtual void powerOnReset() {  write(0);  }
};

class RegistreT1CON : public RegistreSpecial {
protected :
    Timer1* timer1;
public :
    RegistreT1CON(Timer1 *t) : RegistreSpecial("T1CON") { timer1 = t; }
    virtual void write(Byte d);
    virtual void setBit(Byte num);
    virtual void clrBit(Byte num);
//    virtual Byte read() { return data; }
    virtual void powerOnReset() {  write(0xFF);  }
};

class Timer1 : public Evenement
{
protected:
    P18f4550 *processeur;
    bool timerOn, tmr1Output, rd16, extClock, t1run, t1oscen, t1syncN, memoExtClock;
    int16_t prescaler; //limite
    int16_t preCount; // valeur
    int32_t tmr1; // comptage sur 16 bits

    RegistreTMR1L *regTMR1L;
    RegistreTMR1H *regTMR1H;
    RegistreT1CON *regT1CON;

    void compte();
    void calcEvent();

public:
    Timer1(P18f4550 *p);
    virtual ~Timer1() {}

    void config();
    int getTmr1() {  return tmr1;  }  // renvoie TMR1 sur 16 bits
    virtual void executer();
    void setTmr1(Byte d);
    Byte readTmr1();
    void extClockChange(bool val);
    void setTmr1H(Byte d);
    void setBitTmr1H(Byte num);
    void clrBitTmr1H(Byte num);
    Byte readTmr1H();
};

inline void RegistreT1CON::write(Byte d) {  RegistreSpecial::write(d); timer1->config();  }
inline void RegistreT1CON::setBit(Byte num) {  RegistreSpecial::setBit(num); timer1->config();  }
inline void RegistreT1CON::clrBit(Byte num) {  RegistreSpecial::clrBit(num); timer1->config();  }

inline void RegistreTMR1L::write(Byte d) {  timer1->setTmr1(d);  }
inline void RegistreTMR1L::setBit(Byte num) {  timer1->setTmr1(timer1->getTmr1() | maskSet[num]);  }
inline void RegistreTMR1L::clrBit(Byte num) {   timer1->setTmr1(timer1->getTmr1() & maskClr[num]);  }
inline Byte RegistreTMR1L::read() {  return timer1->readTmr1();  }

inline void RegistreTMR1H::write(Byte d) {  timer1->setTmr1H(d);  }
inline void RegistreTMR1H::setBit(Byte num) {  timer1->setBitTmr1H(num);  }
inline void RegistreTMR1H::clrBit(Byte num) {   timer1->clrBitTmr1H(num);  }
inline Byte RegistreTMR1H::read() {  return timer1->readTmr1H();  }

#endif // TIMER1_H
