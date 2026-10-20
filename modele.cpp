#include "modele.h"

#include <math.h>
#include <QDomImplementation>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QDebug>
#include <QThread>

const double Modele::xMin = -5000;
const double Modele::xMax = 5000;
const double Modele::yMin = -5000;
const double Modele::yMax = 5000;

Modele::Modele()
{
    vbat = 12.0;  // tension de batterie
    reduc = 5; // rapport de reduction
    kroul = 0.07;  // coef de resistance au roulement
    masse = 2.0;  // masse de la gamelle en kg
    rayon = 25;  // rayon de la roue en mm
    rendement = 0.9;  // rendement de la transmission
    rdson = 0.54;  // Rds(on) transistor
    vf = 1.0;  // tension de seuil diode

    jackPin = -1;
    fdcPin = -1;
    fdcForce = false;
    for (int i=0; i<3; i++) bpEnfonce[i] = false;
    potCode = 0;

    moteurG = new MoteurCNX(*this, "PWM moteur gauche");
    moteurD = new MoteurCNX(*this, "PWM moteur droit");
    setPwm1aGauche();
    voie = 230.0; // distance entre les deux roues en mm
    x = 0.0;
    y = 0.0;
    theta = 0.0;
    ct = cos(theta);
    st = sin(theta);

    jack.active();

    tabCaptIR.reserve(NB_MAX_CAPT_IR);

    // === ESP32 ===
    esp32 = new Esp32Backend(this);
    groveLcd = new GroveLcdRgb(&lcd, this);
    esp32->setGrove(groveLcd);
    esp32EnCours = false;
    esp32Initialise = false;

    // Charger la bibliotheque QEMU (une seule fois)
#ifdef _WIN32
    QString libPath = QCoreApplication::applicationDirPath()
                      + "/3rdparty/qemu/lib/libqemu-xtensa.dll";
#else
    QString libPath = QCoreApplication::applicationDirPath()
                      + "/3rdparty/qemu/lib/libqemu-xtensa.so";
#endif
    if (esp32->loadLibrary(libPath)) {
        qDebug() << "Modele: bibliotheque QEMU chargee";
    } else {
        qDebug() << "Modele: echec chargement QEMU";
    }

    connect(esp32, &Esp32Backend::gpioChanged,
            this, &Modele::onEsp32Gpio);

    // Fiche jack / fin de course : tout changement d'etat electrique est repercute vers QEMU
    jack.onChange = [this]() { if (jackPin>=0) appliqueContactEsp32(jackPin, jack.niveauHaut()); };
    fdc.onChange  = [this]() { if (fdcPin>=0)  appliqueContactEsp32(fdcPin,  fdc.niveauHaut());  };
    connect(esp32, &Esp32Backend::machineReady, this, &Modele::appliquePinsEsp32);

    connect(esp32, &Esp32Backend::error,
            this, [](const QString &msg) {
                qDebug() << "Modele ESP32 erreur:" << msg;
            });

    connect(groveLcd, &GroveLcdRgb::backlightChanged,
            this, &Modele::backlightChanged);

    // === PWM ESP32 ===
    pwmTimer = new QTimer(this);
    pwmChrono = new QElapsedTimer();
    pwmDernierTemps = 0;
    connect(pwmTimer, &QTimer::timeout, this, &Modele::updatePwm);

    // === Capteurs IR (1 kHz) ===
    irTimer = new QTimer(this);
    connect(irTimer, &QTimer::timeout, this, &Modele::updateIrSensors);
}

Modele::~Modele()
{
    // === PWM ESP32 ===
    if (pwmTimer) {
        pwmTimer->stop();
        delete pwmTimer;
        pwmTimer = nullptr;
    }
    delete pwmChrono;
    pwmChrono = nullptr;

    // === Capteurs IR ===
    if (irTimer) {
        irTimer->stop();
        delete irTimer;
        irTimer = nullptr;
    }

    // === ESP32 ===
    if (esp32) {
        esp32->shutdown();
        delete esp32;
        esp32 = nullptr;
    }
    delete groveLcd;
    groveLcd = nullptr;

    for (unsigned i=0; i<tabCaptIR.size(); i++) delete tabCaptIR[i];
    tabCaptIR.clear();
    delete moteurG;
    delete moteurD;
}

static const int BP_GPIO[3] = { Esp32Pins::BP0, Esp32Pins::BP1, Esp32Pins::BP2 };

void Modele::setBouton(int numero, bool enfonce)
{
    if (numero < 0 || numero > 2) return;
    bpEnfonce[numero] = enfonce;
    if (esp32EnCours) esp32->setDigitalPin(Esp32Pins::pinLogique(BP_GPIO[numero]), enfonce);
}

// Potentiometre sur IO25 (ADC2 canal 8 = entree 16 de QEMU), 0..3,3 V -> 0..4095
void Modele::setPotAN0Value(double value)
{
    potCode = (int)(value / VCC_CARTE * 4095.0);
    if (esp32EnCours) esp32->setAnalogPin(16, potCode);
}

void Modele::appuyeReset()
{
    if (esp32EnCours) resetEsp32();
}

void Modele::relacheReset()
{
    // Rien a faire : le reset ESP32 est declenche sur l'appui
}

int Modele::j2NameToIndex(const QString &name)
{
    for (int i=0; i<Esp32Pins::NB_ENTREES_J2; i++) {
        if (name == j2Name(i)) return i;
    }
    // Ancien format "J2-<broche> (<signal>)"
    if (name.startsWith("J2-")) {
        int broche = name.mid(3).section(' ', 0, 0).toInt();
        for (int i=0; i<Esp32Pins::NB_ENTREES_J2; i++) {
            if (Esp32Pins::ENTREES_J2[i].broche == broche) return i;
        }
    }
    return -1;
}

// Nom affiche et sauvegarde : numero d'IO de l'ESP32 (ex. IO32 pour J2-4)
QString Modele::j2Name(int index)
{
    if (index<0 || index>=Esp32Pins::NB_ENTREES_J2) return Connexion::NCString;
    return QString("IO%1").arg(Esp32Pins::ENTREES_J2[index].gpio);
}

bool Modele::j2Utilise(int index) const
{
    if (index<0) return false;
    if (index==jackPin || index==fdcPin) return true;
    for (const CaptIrCNX *c : tabCaptIR) {
        if (c->getJ2Pin()==index) return true;
    }
    return false;
}

void Modele::getUnusedJ2(std::vector<QString> &tab, std::vector<int> &tabIndex)
{
    for (int i=0; i<Esp32Pins::NB_ENTREES_J2; i++) {
        if (!j2Utilise(i)) {
            tab.push_back(j2Name(i));
            tabIndex.push_back(i);
        }
    }
}

// Envoie le niveau a QEMU pour une entree J2 (GPIO + 1 = numero logique QEMU)
void Modele::appliqueContactEsp32(int indexJ2, bool haut)
{
    if (indexJ2<0 || indexJ2>=Esp32Pins::NB_ENTREES_J2) return;
    if (!esp32 || !esp32->isMachineReady()) return;
    esp32->setDigitalPin(Esp32Pins::pinLogique(Esp32Pins::ENTREES_J2[indexJ2].gpio), haut);
}

// Sortie d'un capteur IR vers QEMU : niveau logique (seuil a mi-tension) et, pour une broche ADC1,
// valeur analogique 12 bits (0 a VCC_CARTE = 3,3 V pleine echelle).
void Modele::appliqueCaptIrEsp32(CaptIrCNX *c, bool force)
{
    int j2 = c->getJ2Pin();
    if (j2<0 || j2>=Esp32Pins::NB_ENTREES_J2) return;
    if (!esp32 || !esp32->isMachineReady()) return;
    const Esp32Pins::EntreeJ2 &e = Esp32Pins::ENTREES_J2[j2];
    double v = qBound(0.0, c->getSrc(), VCC_CARTE);
    int niveau = (v > VCC_CARTE / 2.0) ? 1 : 0;
    int adc = (int)(v / VCC_CARTE * 4095.0 + 0.5);
    if (force || niveau != c->nivEnvoye) {
        esp32->setDigitalPin(Esp32Pins::pinLogique(e.gpio), niveau != 0);
        c->nivEnvoye = niveau;
    }
    if (e.adc >= 0 && (force || adc != c->adcEnvoye)) {
        esp32->setAnalogPin(e.adc, adc);
        c->adcEnvoye = adc;
    }
}

// Broche J2 devenue libre : niveau bas et tension nulle
void Modele::liberePinEsp32(int indexJ2)
{
    if (indexJ2<0 || indexJ2>=Esp32Pins::NB_ENTREES_J2) return;
    appliqueContactEsp32(indexJ2, false);
    int adc = Esp32Pins::ENTREES_J2[indexJ2].adc;
    if (adc>=0 && esp32 && esp32->isMachineReady()) esp32->setAnalogPin(adc, 0);
}

// (Re)positionne toutes les entrees J2 : au demarrage de la machine, apres un reset.
// Une entree non raccordee est forcee a l'etat bas (certaines broches, comme IO35, demarrent a 1 dans QEMU).
void Modele::appliquePinsEsp32()
{
    for (int i=0; i<Esp32Pins::NB_ENTREES_J2; i++) {
        bool haut = false;
        if (i==jackPin) haut = jack.niveauHaut();
        else if (i==fdcPin) haut = fdc.niveauHaut();
        appliqueContactEsp32(i, haut);
        if (!j2Utilise(i)) liberePinEsp32(i);
    }
    for (CaptIrCNX *c : tabCaptIR) appliqueCaptIrEsp32(c, true);

    // Boutons et potentiometre : etat memorise, repris au demarrage et apres un reset
    for (int i=0; i<3; i++) {
        esp32->setDigitalPin(Esp32Pins::pinLogique(BP_GPIO[i]), bpEnfonce[i]);
    }
    esp32->setAnalogPin(16, potCode);
}

bool Modele::jackConnecte(int index)
{
    if (index>=Esp32Pins::NB_ENTREES_J2) return false;
    if (index>=0 && j2Utilise(index) && index!=jackPin) return false;
    if (index==jackPin) return true;
    int ancien = jackPin;
    jackPin = index;
    if (ancien>=0) liberePinEsp32(ancien);   // broche libre : niveau bas
    if (jackPin>=0) appliqueContactEsp32(jackPin, jack.niveauHaut());
    return true;
}

bool Modele::fdcConnecte(int index)
{
    if (index>=Esp32Pins::NB_ENTREES_J2) return false;
    if (index>=0 && j2Utilise(index) && index!=fdcPin) return false;
    if (index==fdcPin) return true;
    int ancien = fdcPin;
    fdcPin = index;
    if (ancien>=0) liberePinEsp32(ancien);
    if (fdcPin>=0) appliqueContactEsp32(fdcPin, fdc.niveauHaut());
    return true;
}

void Modele::saveGamelData(const QString &fichier)
{
    QDomImplementation impl = QDomDocument().implementation();
    // document with document type
    QString name = "gamel";
    QString publicId = "-//YG//DTD Gamel 1.0 //EN";
    QString systemId = "http://www.guinand.fr/DTD/gamel.dtd";
    QDomDocument doc(impl.createDocumentType(name,publicId,systemId));

    // add some XML comment at the beginning
    doc.appendChild(doc.createComment("This file describe a gamel"));
    doc.appendChild(doc.createTextNode("\n")); // for nicer output

    // root node
    QDomElement gamelNode = doc.createElement("gamel");
    gamelNode.setAttribute("name",fichier);
    doc.appendChild(gamelNode);

    // jack
    QDomElement niv1 = creation(doc, gamelNode, "jack");
    creation(doc, niv1, "pullup", jack.getPullUp());
    creation(doc, niv1, "contactno", jack.getContactNO());
    creation(doc, niv1, "resistance", jack.getResistance());
    creation(doc, niv1, "connexion", (jackPin<0)?"NULL":j2Name(jackPin));
    // fdc
    niv1 = creation(doc, gamelNode, "fdc");
    creation(doc, niv1, "pullup", fdc.getPullUp());
    creation(doc, niv1, "contactno", fdc.getContactNO());
    creation(doc, niv1, "resistance", fdc.getResistance());
    creation(doc, niv1, "connexion", (fdcPin<0)?"NULL":j2Name(fdcPin));
    // moteurs
    niv1 = creation(doc, gamelNode, "hacheurs");
    creation(doc, niv1, "pwm1agauche", pwm1aGauche);
    creation(doc, niv1, "vf", getVf());
    creation(doc, niv1, "rdson", getRdson());
    // capteurs
    for (int i=0; i<nbCaptIr(); i++) {
        CaptIrCNX *c = tabCaptIR[i];
        niv1 = creationAttrNumero(doc, gamelNode, "capteur", i);
        creation(doc, niv1, "pullup", c->getPullUp());
        creation(doc, niv1, "nocomp", c->getNoComp());
        creation(doc, niv1, "sup", c->getSup());
        creation(doc, niv1, "rd", c->getRd());
        creation(doc, niv1, "rt", c->getRt());
        creation(doc, niv1, "vseuil", c->getVseuil());
        creation(doc, niv1, "rc", c->getRc());
        creation(doc, niv1, "x", c->getX());
        creation(doc, niv1, "y", c->getY());
        creation(doc, niv1, "h", c->getH());
        creation(doc, niv1, "connexion", (c->getJ2Pin()<0)?"NULL":j2Name(c->getJ2Pin()));
    }
    niv1 = creation(doc, gamelNode, "params");
    creation(doc, niv1, "masse", getMasse());
    creation(doc, niv1, "vbat", getVbat());
    creation(doc, niv1, "reduction", getReduc());
    creation(doc, niv1, "roulement", getKroul());
    creation(doc, niv1, "rayon", getRayon());
    creation(doc, niv1, "rendement", getRendement());
    creation(doc, niv1, "voie", voie);
    creation(doc, niv1, "scotch", CaptIrCNX::getRefScotch());
    creation(doc, niv1, "moquette", CaptIrCNX::getRefMoquette());
    niv1 = creation(doc, gamelNode, "moteur_g");
    creation(doc, niv1, "res", moteurG->getRes());
    creation(doc, niv1, "induc", moteurG->getInduc());
    creation(doc, niv1, "kphi", moteurG->getKphi());
    creation(doc, niv1, "inertie", moteurG->getInertie());
    creation(doc, niv1, "i0", moteurG->getI0());
    niv1 = creation(doc, gamelNode, "moteur_d");
    creation(doc, niv1, "res", moteurD->getRes());
    creation(doc, niv1, "induc", moteurD->getInduc());
    creation(doc, niv1, "kphi", moteurD->getKphi());
    creation(doc, niv1, "inertie", moteurD->getInertie());
    creation(doc, niv1, "i0", moteurD->getI0());
    QFile f(fichier);
    f.open(QIODevice::WriteOnly);
    QTextStream out(&f);
    out << doc.toString();
    f.close();
}

void Modele::decode(const QString &obj, const QString &num, const QString &attr, const QString &val)
{
    if (obj=="jack") {
        if (attr=="pullup") jack.setPullUp(val.toInt());
        else if (attr=="contactno") jack.setContactNO(val.toInt());
        else if (attr=="resistance") jack.setResistance(val.toDouble());
        else if (attr=="connexion") jackConnecte(val);
    } else if (obj=="fdc") {
        if (attr=="pullup") fdc.setPullUp(val.toInt());
        else if (attr=="contactno") fdc.setContactNO(val.toInt());
        else if (attr=="resistance") fdc.setResistance(val.toDouble());
        else if (attr=="connexion") fdcConnecte(val);
    } else if (obj=="hacheurs") {
        if (attr=="pwm1agauche") pwm1aGauche = val.toInt();
        else if (attr=="vf") setVf(val.toDouble());
        else if (attr=="rdson") setRdson(val.toDouble());
    } else if (obj=="capteur") {
        int numero = num.toInt();
        if (numero<0) return;
        if (numero>=NB_MAX_CAPT_IR) return;
        while (numero>=nbCaptIr()) addCaptIr();
        CaptIrCNX *c = tabCaptIR[numero];
        if (attr=="pullup") c->setPullUp(val.toInt());
        else if (attr=="nocomp") c->setNoComp(val.toInt());
        else if (attr=="sup") c->setSup(val.toInt());
        else if (attr=="rd") c->setRd(val.toDouble());
        else if (attr=="rt") c->setRt(val.toDouble());
        else if (attr=="vseuil") c->setVseuil(val.toDouble());
        else if (attr=="rc") c->setRc(val.toDouble());
        else if (attr=="x") c->setX(val.toDouble());
        else if (attr=="y") c->setY(val.toDouble());
        else if (attr=="h") c->setH(val.toDouble());
        else if (attr=="connexion") captIrConnecte(numero, val);
    }  else if (obj=="params") {
        if (attr=="masse") setMasse(val.toDouble());
        else if (attr=="vbat") setVbat(val.toDouble());
        else if (attr=="reduction") setReduc(val.toDouble());
        else if (attr=="roulement") setKroul(val.toDouble());
        else if (attr=="rayon") setRayon(val.toDouble());
        else if (attr=="rendement") setRendement(val.toDouble());
        else if (attr=="voie") voie = val.toDouble();
        else if (attr=="scotch") CaptIrCNX::setRefScotch(val.toDouble());
        else if (attr=="moquette") CaptIrCNX::setRefMoquette(val.toDouble());
    }  else if (obj=="moteur_g") {
        if (attr=="res") moteurG->setRes(val.toDouble());
        else if (attr=="induc") moteurG->setInduc(val.toDouble());
        else if (attr=="kphi") moteurG->setKphi(val.toDouble());
        else if (attr=="inertie") moteurG->setInertie(val.toDouble());
        else if (attr=="i0") moteurG->setI0(val.toDouble());
    }  else if (obj=="moteur_d") {
        if (attr=="res") moteurD->setRes(val.toDouble());
        else if (attr=="induc") moteurD->setInduc(val.toDouble());
        else if (attr=="kphi") moteurD->setKphi(val.toDouble());
        else if (attr=="inertie") moteurD->setInertie(val.toDouble());
        else if (attr=="i0") moteurD->setI0(val.toDouble());
    }
}

void Modele::initialise()
{
    jackDeconnecte();
    fdcDeconnecte();
    while (nbCaptIr()>0) deleteCaptIr(0);
}

void Modele::deleteCaptIr(int num)
{
    if ((num<(int)tabCaptIR.size())&&(num>=0)) {
        CaptIrCNX *c = tabCaptIR.at(num);
        int j2 = c->getJ2Pin();
        c->setJ2Pin(-1);
        liberePinEsp32(j2);
        delete c;
        tabCaptIR.erase(tabCaptIR.begin()+num);
    }
}

void Modele::addCaptIr(int num)
{
    if (tabCaptIR.size()>=NB_MAX_CAPT_IR) return;
    if (num>=(int)tabCaptIR.size()) num=((int)tabCaptIR.size())-1;
    tabCaptIR.push_back(new CaptIrCNX());
    CaptIrCNX *c = tabCaptIR.back();
    c->onChange = [this, c]() { appliqueCaptIrEsp32(c); };
    if (num<0) c->initData(); else c->initData(tabCaptIR.at(num));
    c->forceSurMoquette();
}

bool Modele::captIrConnecte(int num, int index)
{
    if (num<0 || num>=(int)tabCaptIR.size()) return false;
    if (index>=Esp32Pins::NB_ENTREES_J2) return false;
    CaptIrCNX *c = tabCaptIR.at(num);
    if (index==c->getJ2Pin()) return true;
    if (index>=0 && j2Utilise(index)) return false;
    int ancien = c->getJ2Pin();
    c->setJ2Pin(index);
    if (ancien>=0) liberePinEsp32(ancien);
    if (index>=0) appliqueCaptIrEsp32(c, true);
    return true;
}

void Modele::captIrDeconnecte(int num)
{
    captIrConnecte(num, -1);
}

void Modele::gamelSurPiste(qreal xm, qreal ym, qreal thetaDeg)
{
    x = xm;
    y = ym;
    theta = thetaDeg * ( M_PI / 180.0 );
    ct = cos(theta);
    st = sin(theta);
    if (piste.getObstacleCollision(x + 120.0 * ct, y + 120.0 * st)) {
        if (!fdc.isActif()) {
            fdc.active();
            emit fdcChange();
        }
    } else {
        if (fdc.isActif()) {
            fdc.desactive();
            emit fdcChange();
        }
    }
    for (unsigned i=0; i<tabCaptIR.size(); i++) {
        double x0, y0, x1, y1;
        tabCaptIR[i]->unforce();
        x0 = 120.0 + tabCaptIR[i]->getY();
        y0 = -tabCaptIR[i]->getX();
        x1 = x + x0 * ct - y0 * st;
        y1 = y + x0 * st + y0 * ct;
        tabCaptIR[i]->setXPiste(x1);
        tabCaptIR[i]->setYPiste(y1);
        tabCaptIR[i]->evalue(&piste);
    }
    if (tabCaptIR.size()>0) emit captCNYChange();
}

void Modele::setPwm1aGauche(bool gauche)
{
    pwm1aGauche = gauche;
    if (gauche) {
        moteurG->setCnxName("IO26 (PWM1)");
        moteurD->setCnxName("IO27 (PWM2)");
    } else {
        moteurG->setCnxName("IO27 (PWM2)");
        moteurD->setCnxName("IO26 (PWM1)");
    }
    emit cnxMotChange();
}

double CaptIrCNX::ReflexionMoquette = 0.1;
double CaptIrCNX::ReflexionScotch = 0.9;

void CaptIrCNX::preCalcul()
{
    iForward = (VCC_CARTE-0.95)/(5+resD);
    if (iForward>50e-3) iForward=0; // emetteur detruit !
    if (h<0.3) kDist=1.0;
    else kDist = exp(-0.393812354555733*h+0.00471501771834415*h*h+0.000234514291708762*h*h*h);
    ctr =   2.85919974260108*(1-exp(-1.27424205952417*iForward))
          + 2.29365618730621*(1-exp(-0.0962973727877001*iForward))
          - 1.99821120310397*(1-exp(-0.00136052922029612*iForward));
    iC0 = iForward * kDist * ctr;
}

void CaptIrCNX::evalue(Piste *p) // attention, on suppose un scotch de 19 mm
{
    double ic, vce;

    if (forceSurface) {
        if (surfaceLigne) {
            ic = iC0 * ReflexionScotch;
        } else {
            ic = iC0 * ReflexionMoquette;
        }
    } else {
        if (!p) {
            ic = iC0 * ReflexionMoquette;
        } else {
            double dist = p->getScotchDistance(xPiste, yPiste);
            if (dist<0) {
                ic = iC0 * ReflexionMoquette;
            } else {
                double icRel = (ReflexionScotch - ReflexionMoquette) / (1.0 + exp(2.197224577*(dist-9.5)/(400e-3*h+1))) + ReflexionMoquette;
                ic = iC0 * icRel;
            }
        }
    }
    vce = VCC_CARTE - resT * ic;
    if (vce < 0.16) vce = 0.16;
    if (pullUp) v0 = vce; else v0 = VCC_CARTE - vce;
    if (noComp) {
        setSrc(v0, resT);
    } else {
        if (compSup){
            if (v0>vSeuil) setSrc(VCC_CARTE, resC); else setSrc(0.1, 10);
        } else {
            if (v0<vSeuil) setSrc(0.1, 10); else setSrc(VCC_CARTE, resC);
        }
    }
    if (onChange) onChange();
}

MoteurCNX::MoteurCNX(Modele &m, const QString &nom)
{
    modele = &m;
    memo = false; // transistor off
    inertie = 40.0;  // inertie de l'ensemble ramenee sur l'axe moteur g.cm2
    kphi = 25;  // constante de couple du moteur en (N.mm / A) ou (mN.m / A)
    induc = 24e-3;  // inductance du moteur en H
    res = 2.38;  // resistance du moteur
    i0 = 1.5;  // courant à vide sous 12V
    vm = 0;
    im = 0;
    cnxName = nom;
    preCalcul();
}

void MoteurCNX::preCalcul()
{
    double vbat = modele->getVbat(), masse = modele->getMasse(), kroul = modele->getKroul();
    double reduc =modele->getReduc(), rayon = modele->getRayon() * 1e-3, rendement = modele->getRendement();
    double rdson = modele->getRdson(), vf = modele->getVf();
    double inertiekgm2 = inertie * 1e-7 + masse * rayon * rayon / reduc / reduc;
    double kphiNmA = kphi * 1e-3;

    a = rayon * rayon * kroul * 9.81 * masse / ( inertiekgm2 * reduc * reduc ) * 1e3;
    b = rayon * kphiNmA * rendement / ( inertiekgm2 * reduc ) * 1e3;
    c = vbat / induc;
    don = (res + rdson) / induc;
    doff = res / induc;
    f = reduc * kphiNmA / ( induc * rayon ) * 1e-3;
    g = kphiNmA * kphiNmA * rendement * i0 / ( inertiekgm2 * (12 - res * i0) );
    hoff = - vf / induc;
}

void MoteurCNX::modifieRatio(double ratio, double dt)
{
    // Modele moyenne : tension appliquee = ratio * Vbat
    // (valable car frequence PWM >> constantes de temps du moteur)
    if (dt <= 0.0) return;

    double di, dv;

    // Moyenne des equations ON / OFF ponderee par le rapport cyclique
    di = ratio         * (c    - don  * im - f * vm)
         + (1.0 - ratio) * (hoff - doff * im - f * vm);

    dv = b * im - a - g * vm;

    im += di * dt;
    if (im < 0) im = 0.0;

    vm += dv * dt;
    if (vm < 0) vm = 0.0;

    memo = ratio > 0.5;
}


bool Modele::etatPwmMoteur(const MoteurCNX *moteur, double &rapport, double &frequence)
{
    rapport = 0.0;
    frequence = 1000.0;
    if (!esp32EnCours || !esp32) return false;

    // PWM1 = IO26, PWM2 = IO27 ; la gauche est sur PWM1 si pwm1aGauche
    int gpio = ((moteur == moteurG) == pwm1aGauche) ? 26 : 27;
    QMap<int,int> pins = esp32->pwmPins();
    if (!pins.contains(gpio)) return false;   // aucune broche PWM sur ce GPIO
    int canal = pins.value(gpio);
    float duty = esp32->pwmDuty(canal);
    uint32_t freq = esp32->pwmFreq(canal);
    if (duty < 0.0f) return false;
    rapport = qBound(0.0, duty / 100.0, 1.0);
    if (freq > 0) frequence = freq;
    return true;
}

bool Modele::etatPwmGpio(int gpio, double &rapport, double &frequence)
{
    rapport = 0.0;
    frequence = 1000.0;
    if (!esp32EnCours || !esp32) return false;

    QMap<int,int> pins = esp32->pwmPins();
    int canal = pins.value(gpio, -1);
    if (canal >= 0) {
        // Broche en PWM LEDC : rapport cyclique et frequence
        float duty = esp32->pwmDuty(canal);
        uint32_t freq = esp32->pwmFreq(canal);
        if (duty < 0.0f) return false;
        rapport = qBound(0.0, duty / 100.0, 1.0);
        if (freq > 0) frequence = freq;
        return true;
    }

    // Broche en digital output (ou non pilotee) : afficher le niveau courant
    // HIGH -> 3.3 V (rapport 1.0), LOW -> 0 V (rapport 0.0)
    rapport = m_gpioOutputLevels.value(gpio, false) ? 1.0 : 0.0;
    frequence = 0.0;   // signal statique
    return true;
}

void Modele::miseAJourFdc()
{
    if (fdcForce) return;
    if (piste.getObstacleCollision(x + 120.0 * ct, y + 120.0 * st)) {
        if (!fdc.isActif()) {
            fdc.active();
            emit fdcChange();
        }
    } else {
        if (fdc.isActif()) {
            fdc.desactive();
            emit fdcChange();
        }
    }
}

// ============================================================
// === ESP32 ===
// ============================================================

bool Modele::loadEsp32Firmware(const QString &filename)
{
    if (!esp32) return false;
    return esp32->loadFirmware(filename);
}

void Modele::startEsp32()
{
    if (esp32EnCours) return;
    if (!esp32) return;
    lcd.powerOn();
    if (!esp32Initialise) {
        esp32->start();
        esp32Initialise = true;
    } else {
        esp32->resume();
        QThread::msleep(50);
        esp32->reset();
        QTimer::singleShot(300, this, &Modele::appliquePinsEsp32);   // le reset efface l'etat des entrees
    }
    esp32EnCours = true;

    // Demarrer le timer PWM
    if (!pwmChrono->isValid()) {
        pwmChrono->start();
    }
    pwmDernierTemps = pwmChrono->elapsed();
    pwmTimer->start(20);   // 50 Hz : moteurs, odometrie, UI
    irTimer->start(1);     // 1 kHz : capteurs IR + envoi QEMU

}

void Modele::stopEsp32()
{
    if (!esp32EnCours) return;
    if (!esp32) return;
    lcd.powerOff();

    // Arreter le timer PWM
    pwmTimer->stop();
    pwmPinsPrec.clear();

    // Arreter le timer capteurs IR
    irTimer->stop();

    // Pause (le thread QEMU reste vivant)
    esp32->pause();
    esp32EnCours = false;
}

void Modele::resetEsp32()
{
    if (!esp32EnCours) return;
    if (!esp32) return;

    // Reset cote QEMU (CPU + peripheriques ESP32)
    esp32->reset();
    QTimer::singleShot(300, this, &Modele::appliquePinsEsp32);   // le reset efface l'etat des entrees

    // Reset du chrono PWM
    if (pwmChrono->isValid()) {
        pwmDernierTemps = pwmChrono->elapsed();
    }

    // Reset des moteurs
    moteurG->reset();
    moteurD->reset();
}

void Modele::shutdownEsp32()
{
    if (!esp32) return;
    pwmTimer->stop();
    esp32->shutdown();
    esp32Initialise = false;
    esp32EnCours = false;
}

void Modele::onEsp32Gpio(int pin, bool level)
{
    // pin = numero logique QEMU (GPIO + 1)
    int gpio = pin - 1;
    m_gpioOutputLevels[gpio] = level;   // memorise le niveau digital pour l'oscillo

    switch (pin) {
    case Esp32Pins::pinLogique(Esp32Pins::LED_VERTE):   // ledA6 (verte)
        ledA6.setValue(level ? 5.0 : 0.0);
        break;
    case Esp32Pins::pinLogique(Esp32Pins::LED_JAUNE):   // led2 (jaune)
        led2.setValue(level ? 5.0 : 0.0);
        break;
    default:
        break;
    }
}

void Modele::updatePwm()
{
    if (!esp32EnCours || !esp32) return;

    // Calcul du dt reel
    qint64 maintenant = pwmChrono->elapsed();
    double dt = (maintenant - pwmDernierTemps) / 1000.0;
    pwmDernierTemps = maintenant;
    if (dt <= 0.0) return;
    if (dt > 0.2)  dt = 0.2;   // securite si l'app a ete gelee

    // Mapping GPIO -> canal LEDC via le GPIO matrix
    //   GPIO 26 = PWM1, GPIO 27 = PWM2 (canaux 0 et 1 par defaut si non routes)
    QMap<int,int> pins = esp32->pwmPins();
    if (pins != pwmPinsPrec) {
        for (auto it = pins.begin(); it != pins.end(); ++it) {
            if (it.key() != 26 && it.key() != 27)
                qDebug() << "PWM detecte sur GPIO" << it.key() << "(canal LEDC" << it.value() << ")";
        }
        pwmPinsPrec = pins;
    }

    // LED verte (IO4) : sous PWM, la led brille a pleine intensite pendant le rapport
    // cyclique et est eteinte le reste. L'oeil fait la moyenne : luminosite percue
    // proportionnelle au rapport cyclique (0 % = eteinte, 100 % = pleine luminosite).
    if (pins.contains(Esp32Pins::LED_VERTE)) {
        float dutyLed = esp32->pwmDuty(pins.value(Esp32Pins::LED_VERTE));
        if (dutyLed >= 0.0f)
            ledA6.setPwmDuty(dutyLed / 100.0);
    }

    // LED jaune (IO2) : idem sous PWM
    if (pins.contains(Esp32Pins::LED_JAUNE)) {
        float dutyLed = esp32->pwmDuty(pins.value(Esp32Pins::LED_JAUNE));
        if (dutyLed >= 0.0f)
            led2.setPwmDuty(dutyLed / 100.0);
    }

    int ch1 = pins.value(26, 0);
    int ch2 = pins.value(27, 1);
    // Duty en pourcentage (0..100), -1 si indisponible
    float duty1 = esp32->pwmDuty(ch1);
    float duty2 = esp32->pwmDuty(ch2);
    if (duty1 < 0.0f || duty2 < 0.0f) return;
    double ratio1 = qBound(0.0, duty1 / 100.0, 1.0);
    double ratio2 = qBound(0.0, duty2 / 100.0, 1.0);

    // Router vers les bons moteurs selon pwm1aGauche
    double ratioG, ratioD;
    if (pwm1aGauche) {
        ratioG = ratio1;   // moteurG = PWM1 = canal 0
        ratioD = ratio2;   // moteurD = PWM2 = canal 1
    } else {
        ratioG = ratio2;   // moteurG = PWM2 = canal 1
        ratioD = ratio1;   // moteurD = PWM1 = canal 0
    }

    // Faire avancer les moteurs
    if (moteurG) moteurG->modifieRatio(ratioG, dt);
    if (moteurD) moteurD->modifieRatio(ratioD, dt);

    // Mettre a jour la position du gamel
    double dl = 0.5 * (moteurD->getVitesse() + moteurG->getVitesse()) * dt;
    x += dl * ct;
    y += dl * st;
    if (x > xMax) x = xMax; else if (x < xMin) x = xMin;
    if (y > yMax) y = yMax; else if (y < yMin) y = yMin;
    theta += ((moteurD->getVitesse() - moteurG->getVitesse()) / voie * dt);
    ct = cos(theta);
    st = sin(theta);

    // Mise a jour de l'UI
    emit deplaceGamel(x, y, theta);

    // Mise a jour de l'UI (captCNYChange notifie l'UI a 50 Hz ; les valeurs des
    // capteurs sont maintenues a jour par updateIrSensors() a 1 kHz)
    if (tabCaptIR.size() > 0) emit captCNYChange();

    // Collision avec un obstacle : fin de course
    miseAJourFdc();
}

// ============================================================================
// updateIrSensors() : appelee a 1 kHz par irTimer.
// Recalcule la position des capteurs IR dans le repere piste, evalue leur
// reflexion, et envoie la valeur analogique vers QEMU si elle a changed.
// Pas d'emission de signal UI ici (trop frequence pour l'affichage) :
// captCNYChange() est emise par updatePwm() a 50 Hz.
// ============================================================================
void Modele::updateIrSensors()
{
    if (!esp32EnCours || !esp32) return;

    for (unsigned i = 0; i < tabCaptIR.size(); i++) {
        CaptIrCNX *c = tabCaptIR[i];
        c->unforce();
        double x0 = 120.0 + c->getY();
        double y0 = -c->getX();
        double x1 = x + x0 * ct - y0 * st;
        double y1 = y + x0 * st + y0 * ct;
        c->setXPiste(x1);
        c->setYPiste(y1);
        c->evalue(&piste);

        // Envoyer la valeur vers QEMU si elle a change (niveau digital + analogique)
        appliqueCaptIrEsp32(c, false);
    }
}
