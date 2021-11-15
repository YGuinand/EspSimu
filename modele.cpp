#include "modele.h"

#include <math.h>
#include <QDomImplementation>
#include <QFile>
#include <QTextStream>

const double Modele::xMin = -5000;
const double Modele::xMax = 5000;
const double Modele::yMin = -5000;
const double Modele::yMax = 5000;

Modele::Modele() : fdc(*this)
{
    vbat = 12.0;  // tension de batterie
    reduc = 5; // rapport de reduction
    kroul = 0.07;  // coef de resistance au roulement
    masse = 2.0;  // masse de la gamelle en kg
    rayon = 25;  // rayon de la roue en mm
    rendement = 0.9;  // rendement de la transmission
    rdson = 0.54;  // Rds(on) transistor
    vf = 1.0;  // tension de seuil diode

    power = false;

    pic.getPortB()->connecte(3, &bp0);
    pic.getPortB()->connecte(4, &bp1);
    pic.getPortB()->connecte(5, &bp2);

    pic.getPortA()->connecte(0, &potAN0);

    pic.getPortA()->connecte(6, &ledA6);

    pic.getPortD()->connecte(0, lcd.getE());
    pic.getPortD()->connecte(1, lcd.getRS());
    pic.getPortD()->connecte(2, lcd.getRW());
    pic.getPortD()->connecte(4, lcd.getD4());
    pic.getPortD()->connecte(5, lcd.getD5());
    pic.getPortD()->connecte(6, lcd.getD6());
    pic.getPortD()->connecte(7, lcd.getD7());

    jackPin = -1;
    fdcPin = -1;
    fdcForce = false;

    pin[ 0] = new Pin(Port::PORTA, 1, 0);
    pin[ 1] = new Pin(Port::PORTA, 2, 1);
    pin[ 2] = new Pin(Port::PORTA, 3, 2);
    pin[ 3] = new Pin(Port::PORTA, 4, 3);
    pin[ 4] = new Pin(Port::PORTA, 5, 4);
    pin[ 5] = new Pin(Port::PORTB, 0, 5);
    pin[ 6] = new Pin(Port::PORTB, 1, 6);
    pin[ 7] = new Pin(Port::PORTB, 2, 7);
    pin[ 8] = new Pin(Port::PORTC, 0, 8);
    pin[ 9] = new Pin(Port::PORTC, 1, 9);
    pin[10] = new Pin(Port::PORTC, 2, 10);
    pin[11] = new Pin(Port::PORTE, 0, 11);
    pin[12] = new Pin(Port::PORTE, 1, 12);
    pin[13] = new Pin(Port::PORTE, 2, 13);
    nbPins = 14;

    moteurG = new MoteurCNX(*this, "PWM moteur gauche");
    moteurD = new MoteurCNX(*this, "PWM moteur droit");
    pin[ 9]->utilise(true);
    pin[10]->utilise(true);
    setPwm1aGauche();
    voie = 230.0; // distance entre les deux roues en mm
    x = 0.0;
    y = 0.0;
    theta = 0.0;
    ct = cos(theta);
    st = sin(theta);
    temps = 0;

    jack.active();

    tabCaptIR.reserve(NB_MAX_CAPT_IR);
}

Modele::~Modele()
{
    for (int i=0; i<nbPins; i++) delete pin[i];
    for (unsigned i=0; i<tabCaptIR.size(); i++) delete tabCaptIR[i];
    tabCaptIR.clear();
    delete moteurG;
    delete moteurD;
}

void Modele::appuyeReset()
{
    if (!power) return;
    emit setPowerOff();
    pic.powerOnReset();
    lcd.reset();
}

void Modele::relacheReset()
{
    if (!power) return;
    pic.powerOnReset();
    lcd.reset();
    emit setPowerOn();
}

void Modele::powerOn()
{
    power = true;
    lcd.powerOn();
    pic.powerOnReset();
    moteurG->reset();
    moteurD->reset();
    echant = 0;
    if (estEnregistre()) supprimer();
    enregistrer(48000, pic.getListeEvts());
    emit setPowerOn();
}

void Modele::powerOff()
{
    power = false;
    lcd.powerOff();
    pic.powerOff();
    moteurG->reset();
    moteurD->reset();
    emit setPowerOff();
}

int Modele::clock()
{
    int i;

    /*
    for (i=0; i<2500; i++) {
        for (j=0; j<48; j++) {
            pic.clock();
            pic.clock();
            pic.clock();
            pic.clock();
        }
        lcd.clock();
    }
    */
    /*
    i = 0;
    int j = 192;
    do {
        i += pic.clock4();
        if (i>=j) {
            j += 192;
            lcd.clock();
        }
    } while (i<480000);
    */

    i = 0;
    do {
        i += pic.clock4();
        lcd.clock();
    } while (i<480000);

    return i;
}

void Modele::executer()
{
    echant++;
    deplacement();
    if (echant>=50) {
        echant = 0;
        emit deplaceGamel(x, y, theta);
    }
    supprimer();
    enregistrer(48000, pic.getListeEvts());
}

void Modele::getUnusedPins(std::vector<QString> &tab, std::vector<int> &tabIndex)
{
    for (int i=0; i<nbPins; i++) {
        if (!pin[i]->estUtilise()) {
            tab.push_back(pin[i]->getName());
            tabIndex.push_back(i);
        }
    }
}

bool Modele::jackConnecte(int index)
{
    if (index<0) {
        jackDeconnecte();
        return true;
    }
    if (pin[index]->estUtilise()) return false;
    if (jackPin>=0) {
        jack.getPort()->deconnecte(jack.getPin());
        pin[jackPin]->utilise(false);
    }
    pic.getPort(pin[index]->getPort())->connecte(pin[index]->getNum(), &jack);
    pin[index]->utilise(true);
    jackPin = index;
    return true;
}

bool Modele::fdcConnecte(int index)
{
    if (index<0) {
        fdcDeconnecte();
        return true;
    }
    if (pin[index]->estUtilise()) return false;
    if (fdcPin>=0) {
        fdc.getPort()->deconnecte(fdc.getPin());
        pin[fdcPin]->utilise(false);
    }
    pic.getPort(pin[index]->getPort())->connecte(pin[index]->getNum(), &fdc);
    pin[index]->utilise(true);
    fdcPin = index;
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
    creation(doc, niv1, "connexion", (jackPin<0)?"NULL":pin[jackPin]->getName());
    // fdc
    niv1 = creation(doc, gamelNode, "fdc");
    creation(doc, niv1, "pullup", fdc.getPullUp());
    creation(doc, niv1, "contactno", fdc.getContactNO());
    creation(doc, niv1, "resistance", fdc.getResistance());
    creation(doc, niv1, "connexion", (fdcPin<0)?"NULL":pin[fdcPin]->getName());
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
        creation(doc, niv1, "connexion", (c->getPort())?c->getPort()->pinName(c->getPin()):"NULL");
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

int Modele::pinNameToIndex(const QString &name)
{
    for (int i=0; i<nbPins; i++) {
        if (name == pin[i]->getName()) return i;
    }
    return -1;
}

void Modele::deleteCaptIr(int num)
{
    if ((num<(int)tabCaptIR.size())&&(num>=0)) {
        CaptIrCNX *c = tabCaptIR.at(num);
        if (c->getPort()) {
            int index;
            index = pinNameToIndex(c->getPort()->pinName(c->getPin()));
            pin[index]->utilise(false);
        }
        delete c;
        tabCaptIR.erase(tabCaptIR.begin()+num);
    }
}

void Modele::addCaptIr(int num)
{
    if (tabCaptIR.size()>=NB_MAX_CAPT_IR) return;
    if (num>=(int)tabCaptIR.size()) num=((int)tabCaptIR.size())-1;
    tabCaptIR.push_back(new CaptIrCNX(*this));
    CaptIrCNX *c = tabCaptIR.back();
    if (num<0) c->initData(); else c->initData(tabCaptIR.at(num));
    c->forceSurMoquette();
}

bool Modele::captIrConnecte(int num, int index)
{
    if (index<0) {
        captIrDeconnecte(num);
        return true;
    }
    if (pin[index]->estUtilise()) return false;
    CaptIrCNX *c = tabCaptIR.at(num);
    int cnxActuelle = -1;
    if (c->getPort()) cnxActuelle = pinNameToIndex(c->getPort()->pinName(c->getPin()));
    if (cnxActuelle>=0) {
        c->getPort()->deconnecte(c->getPin());
        pin[cnxActuelle]->utilise(false);
    }
    pic.getPort(pin[index]->getPort())->connecte(pin[index]->getNum(), c);
    pin[index]->utilise(true);
    return true;
}

void Modele::captIrDeconnecte(int num)
{
    CaptIrCNX *c = tabCaptIR.at(num);
    int cnxActuelle = -1;
    if (c->getPort()) cnxActuelle = pinNameToIndex(c->getPort()->pinName(c->getPin()));
    if (cnxActuelle>=0) {
        c->getPort()->deconnecte(c->getPin());
        pin[cnxActuelle]->utilise(false);
    }
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
        moteurG->setCnxName("Pin RC2 (PWM1)");
        moteurD->setCnxName("Pin RC1 (PWM2)");
        pic.getPortC()->connecte(2, moteurG);
        pic.getPortC()->connecte(1, moteurD);
    } else {
        moteurG->setCnxName("Pin RC1 (PWM2)");
        moteurD->setCnxName("Pin RC2 (PWM1)");
        pic.getPortC()->connecte(1, moteurG);
        pic.getPortC()->connecte(2, moteurD);
    }
    emit cnxMotChange();
}

void Modele::deplacement()
{
    uint64_t newt = pic.time();
    if (newt == temps) return;
    double dt = ((double)(newt-temps)) / 48e6;
    temps = newt;
    double dl = 0.5 * (moteurD->getVitesse() + moteurG->getVitesse()) * dt;
    x += dl * ct;
    y += dl * st;
    if (x > xMax) x = xMax; else if (x < xMin) x = xMin;
    if (y > yMax) y = yMax; else if (y < yMin) y = yMin;
    theta += ((moteurD->getVitesse() - moteurG->getVitesse()) / voie * dt);
    ct = cos(theta);
    st = sin(theta);
}

double CaptIrCNX::ReflexionMoquette = 0.1;
double CaptIrCNX::ReflexionScotch = 0.9;

void CaptIrCNX::preCalcul()
{
    iForward = (5-1.025)/(8+resD);
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
    vce = 5 - resT * ic;
    if (vce < 0.16) vce = 0.16;
    if (pullUp) v0 = vce; else v0 = 5 - vce;
    if (noComp) {
        setSrc(v0, resT);
    } else {
        if (compSup){
            if (v0>vSeuil) setSrc(5, resC); else setSrc(0.1, 10);
        } else {
            if (v0<vSeuil) setSrc(0.1, 10); else setSrc(5, resC);
        }
    }
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

void MoteurCNX::modifie(double v)
{
    uint64_t newt = modele->time();
    if (newt == temps) return;
    double dt = ((double)(newt-temps)) / 48e6;
    double di, dv;
    temps = newt;
    if (memo) {
        di = c - don * im - f * vm;
    } else {
        di = hoff - doff * im - f * vm;
    }
    dv = b * im - a - g * vm;
    im += (di * dt);
    if (im < 0) im = 0.0;
    vm += (dv * dt);
    if (vm < 0) vm = 0.0;
    memo = v > 3.0;
}

void CaptIrCNX::miseAJour()
{
    double x0, y0, x1, y1, ct, st;

    modele->deplacement();
    ct = modele->getCosTheta();
    st = modele->getSinTheta();
    unforce();
    x0 = 120.0 + getY();
    y0 = -getX();
    x1 = modele->getX() + x0 * ct - y0 * st;
    y1 = modele->getY() + x0 * st + y0 * ct;
    setXPiste(x1);
    setYPiste(y1);
    evalue(&modele->getPiste());
}

void Modele::miseAJourFdc()
{
    deplacement();
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
