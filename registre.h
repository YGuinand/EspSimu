#ifndef REGISTRE_H
#define REGISTRE_H

#include "general.h"
#include "ports.h"
#include "pwm.h"
#include "adc.h"
// #include "datamemory.h"

class Registre {
protected :
    Byte data;

public :
    static const Byte maskSet[8];
    static const Byte maskClr[8];

public :
    Byte readDirect() { return data; }
    void writeDirect(Byte d) { data = d; }
    void incDirect() { data++; }
    void incDirect(Byte add) { data+=add; }
    void decDirect() { data--; }

public :
    Registre() {  data = 0;  }
    virtual ~Registre() {}

    virtual Byte read() { return data; }
    virtual void write(Byte d) { data = d; }
    virtual void setBit(Byte num) { data |= maskSet[num]; }
    virtual void clrBit(Byte num) { data &= maskClr[num]; }
    void bitSetClr(Byte num, bool set) { if (set) this->setBit(num); else this->clrBit(num); }
    virtual void powerOnReset() {  write(0);  }
    virtual string name() { return "NOT A SFR"; }

    virtual void doPreIndirect() {}
    virtual void doPostIndirect() {}
    virtual bool isIndirectReg() {  return false;  }
};

typedef Registre *RegistrePtr;

class RegistreInutilise : public Registre {
public :
    RegistreInutilise() {}
    virtual void write(Byte /* d */) {}
    virtual void setBit(Byte /* num */) {}
    virtual void clrBit(Byte /* num */) {}
    virtual Byte read() { return 0; } // lu comme 0
};

class RegistreSpecial : public Registre {
protected :
    string nom;
public :
    RegistreSpecial(string n) { nom = n; }
    virtual string name() { return nom; }
};

class RegistreStatus : public RegistreSpecial {
public :
    RegistreStatus() : RegistreSpecial("STATUS") {}
    Byte carry() {  return data & 0x01;  }
    Byte digitCarry() {  return (data & 0x02)!=0;  }
    void setCarry(bool c=true) {  if (c) data |= 0x01; else data &= ~0x01;  }
    void setDigitCarry(bool c=true) {  if (c) data |= 0x02; else data &= ~0x02;  }
    Byte negative() {  return (data & 0x10)!=0;  }
    void setNegative(bool c=true) {  if (c) data |= 0x10; else data &= ~0x10;  }
    Byte overflow() {  return (data & 0x08)!=0;  }
    void setOverflow(bool c=true) {  if (c) data |= 0x08; else data &= ~0x08;  }
    Byte zero() {  return (data & 0x04)!=0;  }
    void setZero(bool c=true) {  if (c) data |= 0x04; else data &= ~0x04;  }
    void writeMask(Byte b, Byte m = 0x1F) {  data = ((b & m) | (data & ~m)) & 0x1F;  }
    virtual void write(Byte d) { writeMask(d); }
};

class RegistreBSR : public RegistreSpecial {
public :
    RegistreBSR() : RegistreSpecial("BSR") {}
    virtual void write(Byte d) { data = d & 0x0F; }
};

class RegistreTrisLat : public RegistreSpecial {
protected :
    Port *port;
public :
    RegistreTrisLat(string n, Port *p) : RegistreSpecial(n) {  port = p;  }
    virtual void write(Byte d) {  writeDirect(d); port->changement();  }
    virtual void setBit(Byte num) {  RegistreSpecial::setBit(num); port->changement(num);  }
    virtual void clrBit(Byte num) {  RegistreSpecial::clrBit(num); port->changement(num);  }
};

class RegistrePort : public RegistreSpecial {
protected :
    RegistrePtr lat;
    Port *port;
public :
    RegistrePort(string n, RegistrePtr lreg, Port *p) : RegistreSpecial(n) {  lat = lreg;  port = p;  }
    virtual void write(Byte d) {  lat->write(d);  }
    virtual void setBit(Byte num) {  lat->setBit(num);  }
    virtual void clrBit(Byte num) {  lat->clrBit(num);  }
    virtual Byte read() {  port->miseAJour();  return data;  }
};

class RegistreIndirect : public RegistreSpecial {
protected :
    DataMemory *mem;
public :
    RegistreIndirect(string n, DataMemory *m) : RegistreSpecial(n) {  mem=m;  }
    virtual bool isIndirectReg() {  return true;  }
};

class RegistreIndirect0 : public RegistreIndirect {
public :
    RegistreIndirect0(string n, DataMemory *m) : RegistreIndirect(n, m) {}
    virtual void write(Byte d);
    virtual void setBit(Byte num);
    virtual void clrBit(Byte num);
    virtual Byte read();
};

class RegistreIndirect1 : public RegistreIndirect {
public :
    RegistreIndirect1(string n, DataMemory *m) : RegistreIndirect(n, m) {}
    virtual void write(Byte d);
    virtual void setBit(Byte num);
    virtual void clrBit(Byte num);
    virtual Byte read();
};

class RegistreIndirect2 : public RegistreIndirect {
public :
    RegistreIndirect2(string n, DataMemory *m) : RegistreIndirect(n, m) {}
    virtual void write(Byte d);
    virtual void setBit(Byte num);
    virtual void clrBit(Byte num);
    virtual Byte read();
};

class RegistrePREINC0 : public RegistreIndirect0 {
public :
    RegistrePREINC0(DataMemory *m) : RegistreIndirect0("PREINC0", m) {}
    virtual void doPreIndirect();
};

class RegistrePREINC1 : public RegistreIndirect1 {
public :
    RegistrePREINC1(DataMemory *m) : RegistreIndirect1("PREINC1", m) {}
    virtual void doPreIndirect();
};

class RegistrePREINC2 : public RegistreIndirect2 {
public :
    RegistrePREINC2(DataMemory *m) : RegistreIndirect2("PREINC2", m) {}
    virtual void doPreIndirect();
};

class RegistrePOSTINC0 : public RegistreIndirect0 {
public :
    RegistrePOSTINC0(DataMemory *m) : RegistreIndirect0("POSTINC0", m) {}
    virtual void doPostIndirect();
};

class RegistrePOSTINC1 : public RegistreIndirect1 {
public :
    RegistrePOSTINC1(DataMemory *m) : RegistreIndirect1("POSTINC1", m) {}
    virtual void doPostIndirect();
};

class RegistrePOSTINC2 : public RegistreIndirect2 {
public :
    RegistrePOSTINC2(DataMemory *m) : RegistreIndirect2("POSTINC2", m) {}
    virtual void doPostIndirect();
};

class RegistrePOSTDEC0 : public RegistreIndirect0 {
public :
    RegistrePOSTDEC0(DataMemory *m) : RegistreIndirect0("POSTDEC0", m) {}
    virtual void doPostIndirect();
};

class RegistrePOSTDEC1 : public RegistreIndirect1 {
public :
    RegistrePOSTDEC1(DataMemory *m) : RegistreIndirect1("POSTDEC1", m) {}
    virtual void doPostIndirect();
};

class RegistrePOSTDEC2 : public RegistreIndirect2 {
public :
    RegistrePOSTDEC2(DataMemory *m) : RegistreIndirect2("POSTDEC2", m) {}
    virtual void doPostIndirect();
};

class RegistrePLUSW0 : public RegistreIndirect {
protected :
    Word calcAdr();
public :
    RegistrePLUSW0(DataMemory *m) : RegistreIndirect("PLUSW0", m) {}
    virtual void write(Byte d);
    virtual void setBit(Byte num);
    virtual void clrBit(Byte num);
    virtual Byte read();
};

class RegistrePLUSW1 : public RegistreIndirect {
protected :
    Word calcAdr();
public :
    RegistrePLUSW1(DataMemory *m) : RegistreIndirect("PLUSW1", m) {}
    virtual void write(Byte d);
    virtual void setBit(Byte num);
    virtual void clrBit(Byte num);
    virtual Byte read();
};

class RegistrePLUSW2 : public RegistreIndirect {
protected :
    Word calcAdr();
public :
    RegistrePLUSW2(DataMemory *m) : RegistreIndirect("PLUSW2", m) {}
    virtual void write(Byte d);
    virtual void setBit(Byte num);
    virtual void clrBit(Byte num);
    virtual Byte read();
};

class RegistreADC : public RegistreSpecial {
protected :
    ModuleADC *adc;
public :
    RegistreADC(string n, ModuleADC *a) : RegistreSpecial(n) {  adc=a;  }
};

class RegistreADCON0 : public RegistreADC {
public :
    RegistreADCON0(ModuleADC *a) : RegistreADC("ADCON0", a) {}
    virtual void write(Byte d) {  writeDirect(d); adc->configControl();  }
    virtual void setBit(Byte num) {  RegistreSpecial::setBit(num); adc->configControl();  }
    virtual void clrBit(Byte num) {  RegistreSpecial::clrBit(num); adc->configControl();  }
};

class RegistreADCON1 : public RegistreADC {
public :
    RegistreADCON1(ModuleADC *a) : RegistreADC("ADCON1", a) {}
    virtual void write(Byte d) {  writeDirect(d); adc->configPortsPins();  }
    virtual void setBit(Byte num) {  RegistreSpecial::setBit(num); adc->configPortsPins();  }
    virtual void clrBit(Byte num) {  RegistreSpecial::clrBit(num); adc->configPortsPins();  }
};

class RegistreADCON2 : public RegistreADC {
public :
    RegistreADCON2(ModuleADC *a) : RegistreADC("ADCON2", a) {}
    virtual void write(Byte d) {  writeDirect(d); adc->configClock();  }
    virtual void setBit(Byte num) {  RegistreSpecial::setBit(num); adc->configClock();  }
    virtual void clrBit(Byte num) {  RegistreSpecial::clrBit(num); adc->configClock();  }
};

class RegistreCCPxCON : public RegistreSpecial {
protected :
    Pwm* pwm;
public :
    RegistreCCPxCON(string n, Pwm *p) : RegistreSpecial(n) { pwm = p; }
    virtual void write(Byte d) {  writeDirect(d); pwm->config();  }
    virtual void setBit(Byte num) {  RegistreSpecial::setBit(num); pwm->config();  }
    virtual void clrBit(Byte num) {  RegistreSpecial::clrBit(num); pwm->config();  }
};

class RegistreCCPRxL : public RegistreSpecial {
protected :
    Pwm* pwm;
public :
    RegistreCCPRxL(string n, Pwm *p) : RegistreSpecial(n) { pwm = p; }
    virtual void write(Byte d) {  writeDirect(d); pwm->dutyCycle();  }
    virtual void setBit(Byte num) {  RegistreSpecial::setBit(num); pwm->dutyCycle();  }
    virtual void clrBit(Byte num) {  RegistreSpecial::clrBit(num); pwm->dutyCycle();  }
};

class RegistreCCPRxH : public RegistreSpecial {
protected :
    Pwm* pwm;
public :
    RegistreCCPRxH(string n, Pwm *p) : RegistreSpecial(n) { pwm = p; }
    virtual void write(Byte d) {  if (!pwm->isOn()) writeDirect(d);  }
    virtual void setBit(Byte num) {  if (!pwm->isOn()) RegistreSpecial::setBit(num);  }
    virtual void clrBit(Byte num) {  if (!pwm->isOn()) RegistreSpecial::clrBit(num);  }
};

#endif // REGISTRE_H
