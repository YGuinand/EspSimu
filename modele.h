#ifndef MODELE_H
#define MODELE_H

#include "connexion.h"
#include "lcdscreen.h"
#include "xmlopensavedata.h"
#include "piste.h"

#include <QTimer>
#include <QElapsedTimer>
#include <QMap>
#include <functional>

// === ESP32 ===
#include "esp32backend.h"
#include "grovelcdrgb.h"

class Modele;

class ContactCNX : public Connexion {
private :
    bool pullUp, contactNO, contactActif;
    double res;
public :
    std::function<void()> onChange;   // appele a chaque changement d'etat electrique
    // Niveau logique vu par une entree numerique (seuil a mi-tension)
    bool niveauHaut() {  return getSrc() > VCC_CARTE / 2.0;  }
    void evalue() {
        if (pullUp) {
            if (contactNO) {
                if (contactActif) setSrc(0, 100); else setSrc(VCC_CARTE, res);
            } else {
                if (contactActif) setSrc(VCC_CARTE, res); else setSrc(0, 100);
            }
        } else {
            if (contactNO) {
                if (contactActif) setSrc(VCC_CARTE, 100); else setSrc(0, res);
            } else {
                if (contactActif) setSrc(0, res); else setSrc(VCC_CARTE, 100);
            }
        }
        if (onChange) onChange();
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

class CaptIrCNX : public Connexion {
private :
    bool pullUp, noComp, compSup;
    double resD, resT, resC;
    double vSeuil;
    double x, y, h;

    double xPiste, yPiste;

    double iForward, kDist, ctr, iC0;

    bool forceSurface;
    bool surfaceLigne;
    double v0;

    int j2Pin;   // entree du connecteur J2 (index dans Esp32Pins::ENTREES_J2), -1 = non connecte

    static double ReflexionMoquette;
    static double ReflexionScotch;

    void preCalcul();

public :
    std::function<void()> onChange;   // appele apres chaque evaluation de la sortie
    int nivEnvoye, mvEnvoye;          // dernieres valeurs transmises a QEMU (-1 = jamais)
    int getJ2Pin() const {  return j2Pin;  }
    void setJ2Pin(int p) {  j2Pin = p;  nivEnvoye = -1;  mvEnvoye = -1;  }
    void evalue(Piste *p = NULL);
    void initData() {  x = 0;  y = -4.5;  h = 5;  pullUp = true;  resD = 270;  resT = 33000;  noComp = true;  compSup = true;  resC = 1000;  vSeuil = VCC_CARTE / 2.0;  preCalcul();  evalue();  }
    CaptIrCNX() { j2Pin = -1;  nivEnvoye = -1;  mvEnvoye = -1;  initData(); }
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
    double getVs() {  return getSrc();  }

    void setXPiste(double px) {  xPiste = px;  }
    void setYPiste(double py) {  yPiste = py;  }

    static void setRefScotch(double val) {  ReflexionScotch = val;  }
    static void setRefMoquette(double val) {  ReflexionMoquette = val;  }
    static double getRefScotch() {  return ReflexionScotch;  }
    static double getRefMoquette() {  return ReflexionMoquette;  }

};

class LedCNX : public QObject, public Connexion {
    Q_OBJECT

protected :
    double courant;
    double vSeuil;
    double courantMax;   // courant pour opacite 100 % (affiche : 20 mA)

signals :
    void setCourant(double i);

public :
    LedCNX(double res=175, double seuil=1.5) {  courant=0;  vSeuil=seuil;  courantMax=20e-3;  setSrc(0, res);  }
    virtual void modifie(double v) {  courant=(v>vSeuil) ? (v-vSeuil)/getZ() : 0;  emit setCourant(courant); }
    // PWM : la led brille a pleine intensite pendant le rapport cyclique, eteinte le reste.
    // L'oeil fait la moyenne : luminosite percue = rapport cyclique (0.0 .. 1.0).
    void setPwmDuty(double duty) {  courant=qBound(0.0, duty, 1.0)*courantMax;  emit setCourant(courant); }
    double getCourant() {  return courant;  }
};

class MoteurCNX : public Connexion {
protected :
    Modele *modele;

    double res, induc, kphi, i0;
    double inertie;
    double a,b,c,don,doff,f,g,hoff;

    double im, vm;

    bool memo;

    QString cnxName;

public :
    MoteurCNX(Modele &m, const QString &nom);
    void preCalcul();
    void modifieRatio(double ratio, double dt);
    double getVitesse() {  return vm;  }
    void reset() {  vm = 0.0;  im = 0.0;  }

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

class Modele : public XMLOpenSaveData
{
    Q_OBJECT

protected :
    LedCNX ledA6;
    LedCNX led2;
    LcdScreen lcd;
    ContactCNX jack;
    ContactCNX fdc;
    bool fdcForce;
    int jackPin, fdcPin;   // index dans Esp32Pins::ENTREES_J2 (-1 = non connecte)
    void appliqueContactEsp32(int indexJ2, bool haut);
    void appliqueCaptIrEsp32(CaptIrCNX *c, bool force = false);
    void liberePinEsp32(int indexJ2);
    void setBouton(int numero, bool enfonce);
    bool bpEnfonce[3];   // etat des boutons BP0..BP2
    int potMV;           // derniere tension injectee (mV) du potentiometre
    MoteurCNX *moteurG, *moteurD;
    bool pwm1aGauche;
    std::vector<CaptIrCNX *> tabCaptIR;

    Piste piste;
    double x, y, theta, ct, st;
    double voie;
    double vbat, masse, kroul, reduc, rayon, rendement, rdson, vf;

    // === ESP32 ===
    Esp32Backend *esp32;
    GroveLcdRgb  *groveLcd;
    bool esp32EnCours;
    bool esp32Initialise;
    QString firmwareSelection;   // fichier .bin reellement accepte par le dernier chargement
    QString dernierErreurEsp32;  // texte de la derniere erreur emise par le backend ESP32

    // === PWM ESP32 (50 Hz) ===
    QTimer        *pwmTimer;
    QElapsedTimer *pwmChrono;
    qint64         pwmDernierTemps;
    QMap<int,int>  pwmPinsPrec;   // dernier routage GPIO -> canal LEDC connu
    QMap<int,bool> m_gpioOutputLevels;  // dernier niveau digital connu par GPIO (pour l'oscillo)

    // === Capteurs IR (1 kHz) ===
    QTimer        *irTimer;

signals :
    void fdcChange();
    void captCNYChange();
    void deplaceGamel(double, double, double);
    void cnxMotChange();
    void backlightChanged(int r, int g, int b, int alpha);

public:
    static const double xMin, xMax, yMin, yMax;

    enum { NB_MAX_CAPT_IR = 12 };
    Modele();
    ~Modele();
    // Erreur la plus recente emise par le backend ESP32 (affichage cible dans l'UI,
    // sinon elle n'atteint que qDebug) — vide si aucune erreur ne s'est produite.
    const QString &getErreurEsp32() const { return dernierErreurEsp32; }
    LcdScreen &getLcd() {  return lcd;  }
    LedCNX &getLedA6() {  return ledA6;  }
    LedCNX &getLed2()  {  return led2;   }
    ContactCNX &getJack() {  return jack;  }
    ContactCNX &getFdc() {  return fdc;  }
    // Fiche jack et fin de course : raccordes a une entree du connecteur J2 de la carte ESP32.
    // L'index est celui de Esp32Pins::ENTREES_J2 (-1 = non connecte).
    static int j2NameToIndex(const QString &name);
    static QString j2Name(int index);
    bool j2Utilise(int index) const;
    void getUnusedJ2(std::vector<QString> &tab, std::vector<int> &tabIndex);
    void jackDeconnecte() {  jackConnecte(-1);  }
    bool jackConnecte(int index);
    bool jackConnecte(const QString &name) { return jackConnecte(j2NameToIndex(name)); }
    void fdcDeconnecte() {  fdcConnecte(-1);  }
    bool fdcConnecte(int index);
    bool fdcConnecte(const QString &name) { return fdcConnecte(j2NameToIndex(name)); }
    void appliquePinsEsp32();
    int getJackPinIndex() {  return jackPin;  }
    int getFdcPinIndex() {  return fdcPin;  }
    bool isPwm1aGauche() {  return pwm1aGauche;  }
    void setPwm1aGauche(bool gauche = true);

    int nbCaptIr() {  return tabCaptIR.size();  }
    void deleteCaptIr(int num);
    void addCaptIr(int num=-1);
    CaptIrCNX *getCaptIr(int num) {  return tabCaptIR.at(num);  }
    bool captIrConnecte(int num, int index);
    bool captIrConnecte(int num, const QString &name) { return captIrConnecte(num, j2NameToIndex(name)); }
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

    double getCosTheta() {  return ct;  }
    double getSinTheta() {  return st;  }
    double getX() {  return x;  }
    double getY() {  return y;  }
    void miseAJourFdc();

    // Signal PWM de commande d'un hacheur : rapport cyclique (0..1) et frequence (Hz)
    bool etatPwmMoteur(const MoteurCNX *moteur, double &rapport, double &frequence);
    // Signal PWM sur une broche GPIO quelconque (ex. IO4) : rapport cyclique (0..1) et frequence (Hz)
    bool etatPwmGpio(int gpio, double &rapport, double &frequence);

    // === ESP32 ===
    bool loadEsp32Firmware(const QString &filename);
    void startEsp32();
    void stopEsp32();
    void resetEsp32();
    void shutdownEsp32();
    bool isEsp32Running() const { return esp32EnCours; }
    Esp32Backend *getEsp32() { return esp32; }
    GroveLcdRgb  *getGroveLcd() { return groveLcd; }

public slots :
    void appuyeBP0() {  setBouton(0, true);  }
    void relacheBP0() {  setBouton(0, false);  }
    void appuyeBP1() {  setBouton(1, true);  }
    void relacheBP1() {  setBouton(1, false);  }
    void appuyeBP2() {  setBouton(2, true);  }
    void relacheBP2() {  setBouton(2, false);  }

    void setPotAN0Value(double value);

    void appuyeReset();
    void relacheReset();
    void metJack(bool oui) {  if (oui) jack.active(); else jack.desactive();  }
    void retireJack(bool oui) {  if (oui) jack.desactive(); else jack.active();  }
    void appuyeFdc(bool oui = true) {  fdcForce = true;  if (oui) fdc.active(); else fdc.desactive();  }
    void relacheFdc(bool oui = true) {  fdcForce = false;  if (oui) fdc.desactive(); else fdc.active();  }

    void gamelSurPiste(qreal xm, qreal ym, qreal thetaDeg);

    // === ESP32 ===
    void onEsp32Gpio(int pin, bool level);
    void updatePwm();   // 50 Hz : moteurs, odometrie, UI
    void updateIrSensors();  // 1 kHz : capteurs IR + envoi QEMU
};

#endif // MODELE_H
