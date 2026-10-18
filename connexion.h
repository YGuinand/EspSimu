#ifndef CONNEXION_H
#define CONNEXION_H

#include <QObject>
#include <QString>

// Tension d'alimentation de la carte (ESP32) : niveau haut des entrees, pull-ups, capteurs IR
constexpr double VCC_CARTE = 3.3;

// Source de tension avec impedance : modele electrique d'un element raccorde a la carte
// (LED, contact, capteur IR, hacheur).
class Connexion {
private :
    double valueIn, valueSrc, impedance;
public :
    static const QString NCString;
    Connexion() { valueIn = 0;  valueSrc = 2.5;  impedance = 1e9; }
    virtual ~Connexion() {}
    virtual void modifie(double /* v */) {}
    void setValue(double v) { valueIn = v;  modifie(v); }
    double getZ() const { return impedance; }
    void setSrc(double src, double z) { valueSrc = src;  impedance = z; }
    void setSrc(double src) { valueSrc = src; }
    double getVin() const { return valueIn; }
    double getSrc() const { return valueSrc; }   // tension fournie par la source (avant impedance)

    virtual double getValParam(int /* n */) { return valueIn; }
    virtual const QString getNameParam(int /* n */) { return QString(QObject::tr("Connexion")); }
};

#endif // CONNEXION_H
