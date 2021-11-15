#ifndef TIMER2_H
#define TIMER2_H

#include "general.h"
#include "evenement.h"
#include "registre.h"

class P18f4550;
class Timer2;

class RegistreTMR2 : public RegistreSpecial {
protected :
    Timer2* timer2;
public :
    RegistreTMR2(Timer2 *t) : RegistreSpecial("TMR2") { timer2 = t; }
    virtual void write(Byte d);
    virtual void setBit(Byte num);
    virtual void clrBit(Byte num);
    virtual Byte read();
//    virtual void powerOnReset() {  write(0);  }
};

class RegistrePR2 : public RegistreSpecial {
protected :
    Timer2* timer2;
public :
    RegistrePR2(Timer2 *t) : RegistreSpecial("PR2") { timer2 = t; }
    virtual void write(Byte d);
    virtual void setBit(Byte num);
    virtual void clrBit(Byte num);
//    virtual Byte read() { return data; }
    virtual void powerOnReset() {  write(0xFF);  }
};

class RegistreT2CON : public RegistreSpecial {
protected :
    Timer2* timer2;
public :
    RegistreT2CON(Timer2 *t) : RegistreSpecial("T2CON") { timer2 = t; }
    virtual void write(Byte d);
    virtual void setBit(Byte num);
    virtual void clrBit(Byte num);
//    virtual Byte read() { return data; }
//    virtual void powerOnReset() {  write(0);  }
};

class Timer2 : public Evenement
{
protected:
    P18f4550 *processeur;
    bool timerOn, tmr2Output;
    Byte prescaler, postscaler; //limites
    Byte postCounter; //valeurs
    Word tmr2; // codage sur 14 bits (prescaler intégré)
    Byte pr2;

    RegistreTMR2 *regTMR2;
    RegistrePR2 *regPR2;
    RegistreT2CON *regT2CON;

public:
    Timer2(P18f4550 *p);
    virtual ~Timer2() {}

    void config();
    bool output() {  return tmr2Output;  }
    int getTmr2() {  return tmr2 >> 4;  }  // renvoie TMR2 sur 10 bits
    void setTmr2(Byte d) {  tmr2 = d << 6;  postCounter = 0;  if (estEnregistre()) calcEvent();  }
    void setPr2() {  pr2 = regPR2->read();  }
    Byte getPrescaler() {  return prescaler;  }
    virtual void executer();
    void actualiser();

    void calcEvent();
};

inline void RegistreTMR2::write(Byte d) {  timer2->actualiser();  timer2->setTmr2(d);  }
inline void RegistreTMR2::setBit(Byte num) {  timer2->actualiser();  timer2->setTmr2((timer2->getTmr2() >> 2) | maskSet[num]);  }
inline void RegistreTMR2::clrBit(Byte num) {  timer2->actualiser();  timer2->setTmr2((timer2->getTmr2() >> 2) & maskClr[num]);  }
inline Byte RegistreTMR2::read() {  timer2->actualiser();  return timer2->getTmr2() >> 2;  }

inline void RegistrePR2::write(Byte d) {  timer2->actualiser();  RegistreSpecial::write(d);  timer2->setPr2();  if (timer2->estEnregistre()) timer2->calcEvent();  }
inline void RegistrePR2::setBit(Byte num) {  timer2->actualiser();  RegistreSpecial::setBit(num);  timer2->setPr2();  if (timer2->estEnregistre()) timer2->calcEvent();  }
inline void RegistrePR2::clrBit(Byte num) {  timer2->actualiser();  RegistreSpecial::clrBit(num);  timer2->setPr2();  if (timer2->estEnregistre()) timer2->calcEvent();  }

inline void RegistreT2CON::write(Byte d) {  RegistreSpecial::write(d); timer2->config();  }
inline void RegistreT2CON::setBit(Byte num) {  RegistreSpecial::setBit(num); timer2->config();  }
inline void RegistreT2CON::clrBit(Byte num) {  RegistreSpecial::clrBit(num); timer2->config();  }

#endif // TIMER2_H
