#ifndef MODELE_H
#define MODELE_H

#include "processeur.h"
#include "ports.h"
#include "lcdscreen.h"
#include "xmlopensavedata.h"
#include "piste.h"

class Modele;

class BoutonCNX : public Connexion {
public :
    BoutonCNX() { setSrc(5, 1000); }
    void appuye() { setSrc(0, 100); }
    void relache() { setSrc(5, 1000); }
};

class ContactCNX : public Connexion {
private :
    bool pullUp, contactNO, contactActif;
    double res;
public :
    void evalue() {
        if (pullUp) {
            if (contactNO) {
                if (contactActif) setSrc(0, 100); else setSrc(5, res);
            } else {
                if (contactActif) setSrc(5, res); else setSrc(0, 100);
            }
        } else {
            if (contactNO) {
                if (contactActif) setSrc(5, 100); else setSrc(0, res);
            } else {
                if (contactActif) setSrc(0, res); else setSrc(5, 100);
            }
        }
    }
    ContactCNX(double r=1000) {  res = r;  pullUp = true;  contactNO = true;  contactActif = false;  }
    void setPullUp(bool pu=true) {  pullUp = pu;  evalue();  }
    bool getPullUp() {  return pullUp;  }
    void setContactNO(bool no=true) {  contactNO = no;  evalue();  }
    bool getContactNO() {  return contactNO;  }
    void setResistance(double r) {  res = r;  evalue();  }
    double getResistance() {  return res;  }
    bool isActif() {  return contactActif;  }
    void active() {  contactActif = true;  evalue();  }
    void desactive() {  contactActif = false;  evalue();  }
};

class ContactFdcCNX : public ContactCNX {
protected :
    Modele *modele;
public :
    ContactFdcCNX(Modele &m) : ContactCNX() {  modele = &m;  }
    virtual void miseAJour();
};

class CaptIrCNX : public Connexion {
private :
    bool pullUp, noComp, compSup;
    double resD, resT, resC;
    double vSeuil;
    double x, y, h; // position dans le repere de la gamelle

    double xPiste, yPiste;  // position sur la piste

    double iForward, kDist, ctr, iC0;

    bool forceSurface;
    bool surfaceLigne;
    double v0;

    Modele *modele;

    static double ReflexionMoquette;
    static double ReflexionScotch;

    void preCalcul();

public :
    void evalue(Piste *p = NULL);
    void initData() {  x = 0;  y = -4.5;  h = 5;  pullUp = true;  resD = 270;  resT = 33000;  noComp = true;  compSup = true;  resC = 1000;  vSeuil = 2.5;  preCalcul();  evalue();  }
    CaptIrCNX(Modele &m) { modele = &m;  initData(); }
    bool getPullUp() {  return pullUp;  }
    void setPullUp(bool pu) {  pullUp = pu;  evalue();  }
    double getRd() {  return resD;  }
    void setRd(double r) {  resD = r;  preCalcul();  evalue();  }
    double getRt() {  return resT;  }
    void setRt(double r) {  resT = r;  evalue();  }
    double getRc() {  return resC;  }
    void setRc(double r) {  resC = r;  evalue();  }
    bool getNoComp() {  return noComp;  }
    void setNoComp(double no) {  noComp = no;  evalue();  }
    bool getSup() {  return compSup;  }
    void setSup(bool s) { compSup = s;  evalue();  }
    double getX() {  return x;  }
    void setX(double px) {  x = px;  evalue();  }
    double getY() {  return y;  }
    void setY(double py) {  y = py;  evalue();  }
    double getH() {  return h;  }
    void setH(double ph) {  h = ph;  preCalcul();  evalue();  }
    double getVseuil() {  return vSeuil;  }
    void setVseuil(double v) {  vSeuil = v;  evalue();  }
    void initData(CaptIrCNX *c) { x=c->getX(); y=c->getY(); h=c->getH(); pullUp=c->getPullUp(); resD=c->getRd(); resT=c->getRt();
                                  noComp=c->getNoComp(); compSup=c->getSup(); resC=c->getRc(); vSeuil=c->getVseuil(); preCalcul(); evalue();  }
    double getV0() {  return v0;  }
    void forceSurLigne() {  forceSurface=true;  surfaceLigne=true;  preCalcul();  evalue();  }
    void forceSurMoquette() {  forceSurface=true;  surfaceLigne=false;  preCalcul();  evalue();  }
    bool estSurLigne() {  return surfaceLigne;  }
    void unforce() { forceSurface = false; }
    double getVs() {  if (getPort()) { getPort()->eval(); return getVin(); }   return getValue(0);  }

    void setXPiste(double px) {  xPiste = px;  }
    void setYPiste(double py) {  yPiste = py;  }

    static void setRefScotch(double val) {  ReflexionScotch = val;  }
    static void setRefMoquette(double val) {  ReflexionMoquette = val;  }
    static double getRefScotch() {  return ReflexionScotch;  }
    static double getRefMoquette() {  return ReflexionMoquette;  }

    virtual void miseAJour();
};

class PotCNX : public Connexion {
protected :
    double res;
public :
    PotCNX(double r=1000) {  res=r;  setSrc(0.0,100.0);  }
    void setAlpha(double a) {  setSrc(a*5.0, a*(1-a)*res+100.0);  }
};

class LedCNX : public QObject, public Connexion {
    Q_OBJECT

protected :
    double courant;
    double vSeuil;

signals :
    void setCourant(double i);

public :
    LedCNX(double res=175, double seuil=1.5) {  courant=0;  vSeuil=seuil;  setSrc(0, res);  }
    virtual double getValue(double vpic) {  return (vpic>vSeuil) ? vSeuil : 0;  }
    virtual void modifie(double v) {  courant=(v>vSeuil) ? (v-vSeuil)/getZ() : 0;  emit setCourant(courant); }
    double getCourant() {  return courant;  }
};

class MoteurCNX : public Connexion {
protected :
    Modele *modele;

    double res, induc, kphi, i0;
    double inertie;
    double temps;

    double a,b,c,don,doff,f,g,hoff;

    double im, vm;

    bool memo;

    QString cnxName;

public :
    MoteurCNX(Modele &m, const QString &nom);
    void preCalcul();
    virtual void modifie(double v);
    double getVitesse() {  return vm;  }
    void reset() {  vm = 0.0;  im = 0.0;  }

//    virtual double getValParam(int /* n */) { return getVin(); }
    virtual const QString getNameParam(int /* n */) { return cnxName; }

    void setCnxName(const QString &nom) { cnxName = nom; }
    double getRes() {  return res;  }
    void setRes(double val) {  res = val;  preCalcul();  }
    double getInduc() {  return induc;  }
    void setInduc(double val) {  induc = val;  preCalcul();  }
    double getKphi() {  return kphi;  }
    void setKphi(double val) {  kphi = val;  preCalcul();  }
    double getInertie() {  return inertie;  }
    void setInertie(double val) {  inertie = val;  preCalcul();  }
    double getI0() {  return i0;  }
    void setI0(double val) {  i0 = val;  preCalcul();  }
};

class Pin {
private :
    QString name;
    int port;
    int num;
    int index;
    bool use;
public :
    Pin(int p, int b, int i) {  name = Port::pinName(p, b);  port = p;  num = b;  index = i;  use = false;  }
    bool estUtilise() {  return use;  }
    void utilise(bool u) {  use = u;  }
    int code() {  return port*8+num;  }
    int getPort() {  return port;  }
    int getNum() {  return num;  }
    QString getName() { return name; }
};

class Modele : public XMLOpenSaveData, public Evenement
{
    Q_OBJECT

protected :
    P18f4550 pic;
    BoutonCNX bp0, bp1, bp2;
    LedCNX ledA6;
    PotCNX potAN0;
    LcdScreen lcd;
    ContactCNX jack;
    ContactFdcCNX fdc;
    bool fdcForce;
    int jackPin, fdcPin;
    MoteurCNX *moteurG, *moteurD;
    bool pwm1aGauche;
    vector<CaptIrCNX *> tabCaptIR;

    bool power;

    int nbPins;
    Pin *pin[14];

    Piste piste;
    double x, y, theta, ct, st;
    double voie;
    uint64_t temps;
    uint16_t echant;

    double vbat, masse, kroul, reduc, rayon, rendement, rdson, vf;

    virtual void executer();

signals :
    void setPowerOn();
    void setPowerOff();
    void fdcChange();
    void captCNYChange();
    void deplaceGamel(double, double, double);
    void cnxMotChange();

public:
    static const double xMin, xMax, yMin, yMax;

    enum { NB_MAX_CAPT_IR = 12 };
    Modele();
    ~Modele();
    P18f4550 &getProcesseur() {  return pic;  }
    LcdScreen &getLcd() {  return lcd;  }
    LedCNX &getLedA6() {  return ledA6;  }
    ContactCNX &getJack() {  return jack;  }
    ContactCNX &getFdc() {  return fdc;  }
    int pinNameToIndex(const QString &name);
    void jackDeconnecte() {  if (jackPin>=0) { jack.getPort()->deconnecte(jack.getPin());  pin[jackPin]->utilise(false);  jackPin = -1;  }   }
    bool jackConnecte(int index);
    bool jackConnecte(const QString &name) { return jackConnecte(pinNameToIndex(name)); }
    void fdcDeconnecte() {  if (fdcPin>=0) { fdc.getPort()->deconnecte(fdc.getPin());  pin[fdcPin]->utilise(false);  fdcPin = -1;  }  }
    bool fdcConnecte(int index);
    bool fdcConnecte(const QString &name) { return fdcConnecte(pinNameToIndex(name)); }
    void getUnusedPins(std::vector<QString> &tab, std::vector<int> &tabIndex);
    QString getPinName(int index) {  return pin[index]->getName();  }
    int getJackPinIndex() {  return jackPin;  }
    int getFdcPinIndex() {  return fdcPin;  }
    bool isPwm1aGauche() {  return pwm1aGauche;  }
    void setPwm1aGauche(bool gauche = true);

    int nbCaptIr() {  return tabCaptIR.size();  }
    void deleteCaptIr(int num);
    void addCaptIr(int num=-1);
    CaptIrCNX *getCaptIr(int num) {  return tabCaptIR.at(num);  }
    bool captIrConnecte(int num, int index);
    bool captIrConnecte(int num, const QString &name) { return captIrConnecte(num, pinNameToIndex(name)); }
    void captIrDeconnecte(int num);
    bool captIrAjoutPossible() {  return nbCaptIr()<NB_MAX_CAPT_IR;  }

    void saveGamelData(const QString &fichier);
    virtual void decode(const QString &obj, const QString &num, const QString &attr, const QString &val);
    virtual void initialise();

    Piste &getPiste() { return piste; }

    MoteurCNX *getMoteurG() {  return moteurG;  }
    MoteurCNX *getMoteurD() {  return moteurD;  }

    double getVoie() {  return voie;  }
    void setVoie(double val) {  voie = val;  }

    double getVbat() {  return vbat;  }
    void setVbat(double val) {  vbat = val;  moteurG->preCalcul();  moteurD->preCalcul();  }
    double getMasse() {  return masse;  }
    void setMasse(double val) {  masse = val;  moteurG->preCalcul();  moteurD->preCalcul();  }
    double getKroul() {  return kroul;  }
    void setKroul(double val) {  kroul = val;  moteurG->preCalcul();  moteurD->preCalcul();  }
    double getReduc() {  return reduc;  }
    void setReduc(double val) {  reduc = val;  moteurG->preCalcul();  moteurD->preCalcul();  }
    double getRayon() {  return rayon;  }
    void setRayon(double val) {  rayon = val;  moteurG->preCalcul();  moteurD->preCalcul();  }
    double getRendement() {  return rendement;  }
    void setRendement(double val) {  rendement = val;  moteurG->preCalcul();  moteurD->preCalcul();  }
    double getVf() {  return vf;  }
    void setVf(double val) {  vf = val;  moteurG->preCalcul();  moteurD->preCalcul();  }
    double getRdson() {  return rdson;  }
    void setRdson(double val) {  rdson = val;  moteurG->preCalcul();  moteurD->preCalcul();  }

    uint64_t time() {  return pic.time();  }

    double getCosTheta() {  return ct;  }
    double getSinTheta() {  return st;  }
    double getX() {  return x;  }
    double getY() {  return y;  }
    void deplacement();

    void miseAJourFdc();

public slots :
    void appuyeBP0() {  bp0.appuye();  }
    void relacheBP0() {  bp0.relache();  }
    void appuyeBP1() {  bp1.appuye();  }
    void relacheBP1() {  bp1.relache();  }
    void appuyeBP2() {  bp2.appuye();  }
    void relacheBP2() {  bp2.relache();  }
    void setPotAN0Value(double value) {  potAN0.setAlpha(value/5.0);  }  //0<=value<=5   0<=alpha<=1
    void appuyeReset();
    void relacheReset();
    void powerOn();
    void powerOff();
    int clock();
    void metJack(bool oui) {  if (oui) jack.active(); else jack.desactive();  }
    void retireJack(bool oui) {  if (oui) jack.desactive(); else jack.active();  }
    void appuyeFdc(bool oui = true) {  fdcForce = true;  if (oui) fdc.active(); else fdc.desactive();  }
    void relacheFdc(bool oui = true) {  fdcForce = false;  if (oui) fdc.desactive(); else fdc.active();  }

    void gamelSurPiste(qreal xm, qreal ym, qreal thetaDeg);
};

inline void ContactFdcCNX::miseAJour() {  modele->miseAJourFdc();  }

#endif // MODELE_H
