#ifndef PORTS_H
#define PORTS_H

#include "general.h"

class P18f4550;
class DataMemory;
class Port;

class Connexion {
private :  
    double valueIn, valueSrc, impedance;
    Port *port;
    Byte pin;
public :
    static const QString NCString;
    Connexion() { valueIn = 0;  valueSrc = 2.5;  impedance = 1e9;  port = NULL; }
    virtual ~Connexion();
    virtual void modifie(double /* v */) {}
    void setValue(double v) { valueIn = v;  modifie(v); }
    virtual double getValue(double /* vpic */) { return valueSrc; }
    double getZ() { return impedance; }
    void setZ(double z);
    void setSrc(double src, double z);
    void setSrc(double src);
    void setLow();
    void setHigh();
    void setOutput(bool high=false);
    void setInput();
    void setPort(Port *p, Byte n) {  port = p;  pin = n;  }
    void changement();

    Port *getPort() const { return port; }
    Byte getPin() const { return pin; }
    double getVin() const { return valueIn; }

    virtual double getValParam(int /* n */) { return valueIn; }
    virtual const QString getNameParam(int /* n */) { return QString(QObject::tr("Connexion")); }

    const QString pinName() const;

    virtual void miseAJour() {}
};

class Port {
private :
    bool globalChange;
    Connexion *cnx[8];
protected :
    P18f4550 *processeur;
    bool analogPin[8];
    bool change[8];
    virtual void evalue() = 0;
public :
    enum { PORTA=0, PORTB, PORTC, PORTD, PORTE };
    Port(P18f4550 *p);
    DataMemory *getDM();
    bool getTTL(double v);
    void setBit(Byte &val, Byte n, bool s);
    double setCnxValue(Byte n, Byte reg, bool input=false);
    void changement() { change[0] = change[1] = change[2] = change[3] = change[4] = change[5] = change[6] = change[7] = true;  globalChange = true; }
    void changement(Byte pin) { change[pin] = true;  globalChange = true; }
    void deconnecte(Byte pin) {  if (pin<8) if (cnx[pin]) { cnx[pin]->setPort(NULL, 0); cnx[pin] = NULL; changement(pin); }  }
    void connecte(Byte pin, Connexion *c) {  deconnecte(pin); if (pin<8) { cnx[pin] = c; c->setPort(this, pin); changement(pin); } }
    void eval() { if (globalChange) { evalue();  globalChange = false; } }
    virtual void analogConfig() {}
    double getAnalogValue(Byte pin, double vpic, double tSample);
    bool estConnecte(Byte pin) {  if (pin<8) return cnx[pin]!=NULL;  return true;  }
    virtual int getNumero() = 0;
    static QString pinName(int p, Byte pin) {  if ((pin<8)&&(p>=PORTA)&&(p<=PORTE)) return QString("R")+QChar('A'+p)+QString().setNum(pin); else return QString();  }
    QString pinName(Byte pin) {  return pinName(getNumero(), pin);  }
    void miseAJour() {  for (int i=0; i<8; i++) if (cnx[i]) cnx[i]->miseAJour();  }
};

inline void Connexion::setZ(double z) { impedance = z;  changement(); }
inline void Connexion::setSrc(double src, double z) { valueSrc = src;  impedance = z;  changement(); }
inline void Connexion::setSrc(double src) { valueSrc = src;  changement(); }
inline void Connexion::setLow() { valueSrc = 0;  changement(); }
inline void Connexion::setHigh() { valueSrc = 5;  changement(); }
inline void Connexion::setOutput(bool high) { valueSrc = high ? 5 : 0;  impedance = 1000;  changement(); }
inline void Connexion::setInput() { impedance = 1e9;  valueSrc = 2.5;  changement(); }
inline void Connexion::changement() { if (port != NULL) port->changement(pin); }
inline Connexion::~Connexion() {  if (port != NULL) port->deconnecte(pin);  port=NULL;  }
inline const QString Connexion::pinName() const { if (port) return port->pinName(pin); else return NCString; }

class PortA : public Port {
public :
    PortA(P18f4550 *p) : Port(p) {}
    virtual void analogConfig();
    virtual int getNumero() {  return PORTA;  }
protected :
    virtual void evalue();
};

class PortB : public Port {
public :
    PortB(P18f4550 *p) : Port(p) {}
    virtual void analogConfig();
    virtual int getNumero() {  return PORTB;  }
protected :
    virtual void evalue();
};

class PortC : public Port {
public :
    PortC(P18f4550 *p) : Port(p) {}
    virtual int getNumero() {  return PORTC;  }
protected :
    virtual void evalue();
};

class PortD : public Port {
public :
    PortD(P18f4550 *p) : Port(p) {}
    virtual int getNumero() {  return PORTD;  }
protected :
    virtual void evalue();
};

class PortE : public Port {
public :
    PortE(P18f4550 *p) : Port(p) {}
    virtual void analogConfig();
    virtual int getNumero() {  return PORTE;  }
protected :
    virtual void evalue();
};

#endif // PORTS_H
