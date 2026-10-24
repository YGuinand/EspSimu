#include "piste.h"
#include "modele.h"

#include <QGraphicsRectItem>
#include <QPainterPath>
#include <QFile>

Portion::Portion(int n, Portion *src)
{
    id = n;
    noSrc = false;
    item = NULL;
    itemFocus = NULL;
    itemCalc = false;
    focus = false;
    scene = NULL;
    source = src;
}

Portion::~Portion()
{
    enleverDeScene();
    if (item) delete item;
    if (itemFocus) delete itemFocus;
}

void Portion::calculPosition()
{
    if (!noSrc) {
        if (source) {
            x0 = source->getX();
            y0 = source->getY();
            theta0 = source->getTheta();
            cosTheta0 = cos(theta0);
            sinTheta0 = sin(theta0);
        } else {
            x0 = 0;
            y0 = 0;
            theta0 = 0;
            cosTheta0 = 1;
            sinTheta0 = 0;
        }
    }
    if (itemCalc) moveItem(); else calculItem();
}

void Portion::moveItem()
{
    double thetaD = -theta0*(180.0/M_PI);

    if (item) {
        item->setRotation(thetaD);
        item->setPos(x0, -y0);
    }
    if (itemFocus) {
        itemFocus->setRotation(thetaD);
        itemFocus->setPos(x0, -y0);
    }
}

void Portion::ajouterDansScene(QGraphicsScene &s)
{
    calculItem();
    if (item) s.addItem(item);
    if (itemFocus) s.addItem(itemFocus);
    scene = &s;
}

void PortionRepere::calculItem()
{

    QGraphicsPolygonItem *triangle;

    if (!itemFocus) {
        QPolygonF polygon;
        polygon << QPointF(0,0) << QPointF(-100,100) << QPointF(200,0) << QPointF(-100,-100) << QPointF(0,0);
        triangle = new QGraphicsPolygonItem(polygon);
        itemFocus = triangle;
    } else {
        triangle = dynamic_cast<QGraphicsPolygonItem *>(itemFocus);
    }
    triangle->setBrush(Qt::red);
    triangle->setPen(Qt::NoPen);
    triangle->setRotation(-theta0*180/M_PI);
    triangle->setPos(x0, -y0);
    triangle->setZValue(2.0);
    triangle->setVisible(focus);

    Portion::calculItem();
}

QString PortionRepere::nameArg(int n)
{
    switch (n) {
    case 0 :  return QString("x");
    case 1 :  return QString("y");
    case 2 :  return QString("angle");
    }
    return QString("rien");
}

double PortionRepere::valArg(int n)
{
    switch (n) {
    case 0 :  return x0;
    case 1 :  return y0;
    case 2 :  return theta0 * (180.0/M_PI);
    }
    return 0.0;
}

double PortionRepere::maxArg(int n)
{
    switch (n) {
    case 0 :  return Modele::xMax;
    case 1 :  return Modele::yMax;
    case 2 :  return 360.0;
    }
    return 0.0;
}

double PortionRepere::minArg(int n)
{
    switch (n) {
    case 0 :  return Modele::xMin;
    case 1 :  return Modele::yMin;
    case 2 :  return -360.0;
    }
    return 0.0;
}

void PortionRepere::modifArg(int n, double val)
{
    switch (n) {
    case 0 :  x0 = val;  break;
    case 1 :  y0 = val;  break;
    case 2 :  theta0 = val * (M_PI/180.0);  break;
    }
    calculItem();
}

void PortionLigneDroite::calculItem()
{
    QGraphicsRectItem *ligne;

    if (!item) {
        ligne = new QGraphicsRectItem(0, -largeur/2, longueur, largeur);
        item = ligne;
    } else {
        ligne = dynamic_cast<QGraphicsRectItem *>(item);
        ligne->setRect(0, -largeur/2, longueur, largeur);
    }
    ligne->setBrush(Qt::white);
    ligne->setPen(Qt::NoPen);
    ligne->setRotation(-theta0*180/M_PI);
    ligne->setPos(x0, -y0);

    if (!itemFocus) {
        ligne = new QGraphicsRectItem(0, -largeur/2, longueur, largeur);
        itemFocus = ligne;
    } else {
        ligne = dynamic_cast<QGraphicsRectItem *>(itemFocus);
        ligne->setRect(0, -largeur/2, longueur, largeur);
    }
    ligne->setBrush(Qt::red);
    ligne->setPen(Qt::NoPen);
    ligne->setRotation(-theta0*180/M_PI);
    ligne->setPos(x0, -y0);
    ligne->setZValue(2.0);
    ligne->setVisible(focus);

    Portion::calculItem();
}

double PortionLigneDroite::getScotchDistance(double x, double y)
{
    x -= x0;
    y -= y0;
    double x1, y1, dist;
    x1 = x * cosTheta0 + y * sinTheta0;
    y1 = y * cosTheta0 - x * sinTheta0;
    if (x1 < 0) {
        dist = -1; // hypot(x1, y1); // pas de dist / normale
    } else if (x1 > longueur) {
        dist = -1; // hypot(x1-longueur, y1); // pas de dist / normale
    } else {
        dist = fabs(y1);
    }
    return dist;
}

void PortionLigneAveugle::calculItem()
{
    QGraphicsRectItem *ligne;

    if (!itemFocus) {
        ligne = new QGraphicsRectItem(0, -8, longueur, 16);
        itemFocus = ligne;
    } else {
        ligne = dynamic_cast<QGraphicsRectItem *>(itemFocus);
        ligne->setRect(0, -8, longueur, 16);
    }
    ligne->setBrush(Qt::red);
    ligne->setPen(Qt::NoPen);
    ligne->setRotation(-theta0*180/M_PI);
    ligne->setPos(x0, -y0);
    ligne->setZValue(2.0);
    ligne->setVisible(focus);

    Portion::calculItem();
}

double PortionLigneCourbe::getX()
{
    if (angle > 0.0) return x0 + rayon * (sin(theta0 + angle) - sin(theta0));
    else return x0 + rayon * (-sin(theta0 + angle) + sin(theta0));
}

double PortionLigneCourbe::getY()
{
    if (angle > 0) return y0 + rayon * (cos(theta0) - cos(theta0 + angle));
    else return y0 + rayon * (-cos(theta0) + cos(theta0 + angle));
}

void PortionLigneCourbe::calculItem()
{
    QPainterPath path;
    QGraphicsPathItem * itemPath;
    double l2 = largeur/2.0, r2 = rayon*2.0;
    double aDeg = angle * (180.0 / M_PI);

    if (aDeg<0) {
        path.moveTo(0, l2);
        path.arcTo(-rayon+l2, l2, r2-largeur, r2-largeur, 90, aDeg);
        path.arcTo(-rayon-l2, -l2, r2+largeur, r2+largeur, 90+aDeg, -aDeg);
    } else {
        path.moveTo(0, l2);
        path.arcTo(-rayon+l2, -r2+l2, r2-largeur, r2-largeur, -90, aDeg);
        path.arcTo(-rayon-l2, -r2-l2, r2+largeur, r2+largeur, -90+aDeg, -aDeg);
    }

    if (!item) {
        itemPath = new QGraphicsPathItem(path);
        item = itemPath;
    } else {
        itemPath = dynamic_cast<QGraphicsPathItem *>(item);
        itemPath->setPath(path);
    }
    itemPath->setBrush(Qt::white);
    itemPath->setPen(Qt::NoPen);
    itemPath->setRotation(-theta0*(180/M_PI));
    itemPath->setPos(x0, -y0);

    if (!itemFocus) {
        itemPath = new QGraphicsPathItem(path);
        itemFocus = itemPath;
    } else {
        itemPath = dynamic_cast<QGraphicsPathItem *>(itemFocus);
        itemPath->setPath(path);
    }
    itemPath->setBrush(Qt::red);
    itemPath->setPen(Qt::NoPen);
    itemPath->setRotation(-theta0*(180/M_PI));
    itemPath->setPos(x0, -y0);
    itemPath->setZValue(2.0);
    itemPath->setVisible(focus);

    Portion::calculItem();
}

double PortionLigneCourbe::getScotchDistance(double x, double y)
{
    x -= x0;
    y -= y0;
    double x1, y1, dist1, dist2;
    x1 = x * cosTheta0 + y * sinTheta0;
    y1 = y * cosTheta0 - x * sinTheta0;
    double alpha;
    if (angle < 0) {
        alpha = -angle;
        y1 = -y1;
    } else {
        alpha = angle;
    }
    y1 -= rayon; // ramene le cercle au centre du repere

    double tn = atan2(x1, -y1);
    if (tn < 0) tn += M_PI;
    if (tn > alpha) { // intersection hors arc
        dist1 = -1; // pas de dist / normale
    } else {
        double rSinTn = rayon * sin(tn), rCosTn = rayon * cos(tn);
        dist1 = hypot(x1-rSinTn, y1+rCosTn);
        tn += M_PI;
        if (tn <= alpha) {
            dist2 = hypot(x1+rSinTn, y1-rCosTn);
            if (dist2 < dist1) dist1 = dist2;
        } else {
            // pas de 2nde dist / normale
        }
    }
    return dist1;
}

QString PortionLigneCourbe::nameArg(int n)
{
    switch (n) {
    case 0 :  return QString("rayon");
    case 1 :  return QString("angle");
     }
    return QString("rien");
}

double PortionLigneCourbe::valArg(int n)
{
    switch (n) {
    case 0 :  return rayon;
    case 1 :  return angle * (180.0/M_PI);
    }
    return 0.0;
}

double PortionLigneCourbe::maxArg(int n)
{
    switch (n) {
    case 0 :  return Modele::xMax-Modele::xMin;
    case 1 :  return 360.0;
    }
    return 0.0;
}

double PortionLigneCourbe::minArg(int n)
{
    switch (n) {
    case 0 :  return 0.0;
    case 1 :  return -360.0;
    }
    return 0.0;
}

void PortionLigneCourbe::modifArg(int n, double val)
{
    switch (n) {
    case 0 :  rayon = val;  break;
    case 1 :  angle = val * (M_PI/180.0);  break;
    }
    calculItem();
}

void PortionObstacle::calculItem()
{
    QGraphicsRectItem *obs;

    if (!item) {
        obs = new QGraphicsRectItem(-largeur/2, -longueur/2, largeur, longueur);
        item = obs;
    } else {
        obs = dynamic_cast<QGraphicsRectItem *>(item);
        obs->setRect(-largeur/2, -longueur/2, largeur, longueur);
    }
    obs->setBrush(QBrush(QPixmap(":/images/wood.png")));
    obs->setPen(Qt::NoPen);
    obs->setRotation(-theta0*(180.0/M_PI));
    obs->setPos(x0, -y0);

    if (!itemFocus) {
        obs = new QGraphicsRectItem(-largeur/2, -longueur/2, largeur, longueur);
        itemFocus = obs;
    } else {
        obs = dynamic_cast<QGraphicsRectItem *>(itemFocus);
        obs->setRect(-largeur/2, -longueur/2, largeur, longueur);
    }
    obs->setBrush(Qt::red);
    obs->setPen(Qt::NoPen);
    obs->setRotation(-theta0*(180.0/M_PI));
    obs->setPos(x0, -y0);
    obs->setZValue(2.0);
    obs->setVisible(focus);

    Portion::calculItem();
}

bool PortionObstacle::getObstacleCollision(double x, double y)
{
    x -= x0;
    y -= y0;
    double x1, y1;
    x1 = x * cosTheta0 + y * sinTheta0;
    y1 = y * cosTheta0 - x * sinTheta0;
    x1 += largeur/2;
    if (x1<0) return false;
    if (x1>largeur) return false;
    y1 += longueur/2;
    if (y1<0) return false;
    if (y1>longueur) return false;
    return true;
}


QString PortionObstacle::nameArg(int n)
{
    switch (n) {
    case 0 :  return QString("longueur");
    case 1 :  return QString("largeur");
     }
    return QString("rien");
}

double PortionObstacle::valArg(int n)
{
    switch (n) {
    case 0 :  return longueur;
    case 1 :  return largeur;
    }
    return 0.0;
}

double PortionObstacle::maxArg(int n)
{
    switch (n) {
    case 0 :  return Modele::xMax-Modele::xMin;
    case 1 :  return Modele::xMax-Modele::xMin;
    }
    return 0.0;
}

double PortionObstacle::minArg(int n)
{
    switch (n) {
    case 0 :  return 0.0;
    case 1 :  return 0.0;
    }
    return 0.0;
}

void PortionObstacle::modifArg(int n, double val)
{
    switch (n) {
    case 0 :  longueur = val;  break;
    case 1 :  largeur = val;  break;
    }
    calculItem();
}

void PortionMarqueurGauche::calculItem()
{
    QGraphicsRectItem *ligne;

    if (!item) {
        ligne = new QGraphicsRectItem(-largeur/2, -longueur,  largeur, longueur);
        item = ligne;
    } else {
        ligne = dynamic_cast<QGraphicsRectItem *>(item);
        ligne->setRect(-largeur/2, -longueur,  largeur, longueur);
    }
    ligne->setBrush(Qt::white);
    ligne->setPen(Qt::NoPen);
    ligne->setRotation(-theta0*180/M_PI);
    ligne->setPos(x0, -y0);

    if (!itemFocus) {
        ligne = new QGraphicsRectItem(-largeur/2, -longueur,  largeur, longueur);
        itemFocus = ligne;
    } else {
        ligne = dynamic_cast<QGraphicsRectItem *>(itemFocus);
        ligne->setRect(-largeur/2, -longueur,  largeur, longueur);
    }
    ligne->setBrush(Qt::red);
    ligne->setPen(Qt::NoPen);
    ligne->setRotation(-theta0*180/M_PI);
    ligne->setPos(x0, -y0);
    ligne->setZValue(2.0);
    ligne->setVisible(focus);

    Portion::calculItem();
}

double PortionMarqueurGauche::getScotchDistance(double x, double y)
{
    x -= x0;
    y -= y0;
    double x1, y1, dist;
    x1 = x * cosTheta0 + y * sinTheta0;
    y1 = y * cosTheta0 - x * sinTheta0;
    if (y1 < 0) {
        dist = -1; // hypot(x1, y1); // pas de dist / normale
    } else if (y1 > longueur) {
        dist = -1; // hypot(x1-longueur, y1); // pas de dist / normale
    } else {
        dist = fabs(x1);
    }
    return dist;
}

Piste::Piste()
{
    scene = NULL;
    focus = -1;
}

Piste::~Piste()
{
    for (unsigned int i=0; i<tab.size(); i++) delete tab[i];
    tab.clear();
}

void Piste::initialise()
{
    focus = -1;
    for (unsigned int i=0; i<tab.size(); i++) delete tab[i];
    tab.clear();
}

void Piste::ajouterDansScene(QGraphicsScene &s)
{
    scene = &s;
    for (unsigned int i=0; i<tab.size(); i++) tab[i]->ajouterDansScene(s);
}

template <typename T>T &Piste::getPortion(int n) {
    for (unsigned int i=0; i<tab.size(); i++) {
        if (tab[i]->getID()==n) return *dynamic_cast<T *>(tab[i]);
    }
    T *p = new T(n);
    ajoute(*p);
    return *p;
}

void Piste::decode(const QString &obj, const QString &num, const QString &attr, const QString &val)
{
    if (obj=="repere") {
        if (attr=="x") getPortion<PortionRepere>(num.toInt()).setX(val.toDouble());
        else if (attr=="y") getPortion<PortionRepere>(num.toInt()).setY(val.toDouble());
        else if (attr=="theta") getPortion<PortionRepere>(num.toInt()).setThetaDeg(val.toDouble());
    } else if (obj=="droite") {
        if (attr=="longueur") getPortion<PortionLigneDroite>(num.toInt()).setLongueur(val.toDouble());
        else if (attr=="largeur") getPortion<PortionLigneDroite>(num.toInt()).setLargeur(val.toDouble());
    } else if (obj=="aveugle") {
        if (attr=="longueur") getPortion<PortionLigneAveugle>(num.toInt()).setLongueur(val.toDouble());
    } else if (obj=="obstacle") {
        if (attr=="longueur") getPortion<PortionObstacle>(num.toInt()).setLongueur(val.toDouble());
        else if (attr=="largeur") getPortion<PortionObstacle>(num.toInt()).setLargeur(val.toDouble());
    } else if (obj=="courbe") {
        if (attr=="rayon") getPortion<PortionLigneCourbe>(num.toInt()).setRayon(val.toDouble());
        else if (attr=="largeur") getPortion<PortionLigneCourbe>(num.toInt()).setLargeur(val.toDouble());
        else if (attr=="angle") getPortion<PortionLigneCourbe>(num.toInt()).setAngleDeg(val.toDouble());
    } else if (obj=="marqueur_gauche") {
        if (attr=="longueur") getPortion<PortionMarqueurGauche>(num.toInt()).setLongueur(val.toDouble());
        else if (attr=="largeur") getPortion<PortionMarqueurGauche>(num.toInt()).setLargeur(val.toDouble());
    }
}

void PortionRepere::saveXML(QDomDocument &doc, QDomElement &pNode)
{
    QDomElement niv1 = XMLOpenSaveData::creationAttrNumero(doc, pNode, "repere", id);
    XMLOpenSaveData::creation(doc, niv1, "x", x0);
    XMLOpenSaveData::creation(doc, niv1, "y", y0);
    XMLOpenSaveData::creation(doc, niv1, "theta", theta0*180.0/M_PI);
}

void PortionLigneDroite::saveXML(QDomDocument &doc, QDomElement &pNode)
{
    QDomElement niv1 = XMLOpenSaveData::creationAttrNumero(doc, pNode, "droite", id);
    XMLOpenSaveData::creation(doc, niv1, "longueur", longueur);
    XMLOpenSaveData::creation(doc, niv1, "largeur", largeur);
}

void PortionLigneAveugle::saveXML(QDomDocument &doc, QDomElement &pNode)
{
    QDomElement niv1 = XMLOpenSaveData::creationAttrNumero(doc, pNode, "aveugle", id);
    XMLOpenSaveData::creation(doc, niv1, "longueur", longueur);
}

void PortionObstacle::saveXML(QDomDocument &doc, QDomElement &pNode)
{
    QDomElement niv1 = XMLOpenSaveData::creationAttrNumero(doc, pNode, "obstacle", id);
    XMLOpenSaveData::creation(doc, niv1, "longueur", longueur);
    XMLOpenSaveData::creation(doc, niv1, "largeur", largeur);
}

void PortionLigneCourbe::saveXML(QDomDocument &doc, QDomElement &pNode)
{
    QDomElement niv1 = XMLOpenSaveData::creationAttrNumero(doc, pNode, "courbe", id);
    XMLOpenSaveData::creation(doc, niv1, "rayon", rayon);
    XMLOpenSaveData::creation(doc, niv1, "largeur", largeur);
    XMLOpenSaveData::creation(doc, niv1, "angle", angle*180.0/M_PI);
}

void PortionMarqueurGauche::saveXML(QDomDocument &doc, QDomElement &pNode)
{
    QDomElement niv1 = XMLOpenSaveData::creationAttrNumero(doc, pNode, "marqueur_gauche", id);
    XMLOpenSaveData::creation(doc, niv1, "longueur", longueur);
    XMLOpenSaveData::creation(doc, niv1, "largeur", largeur);
}

void Piste::saveData(const QString &fichier)
{
    QDomImplementation impl = QDomDocument().implementation();
    // document with document type
    QString name = "piste";
    QString publicId = "-//YG//DTD Piste 1.0 //EN";
    QString systemId = "http://www.guinand.fr/DTD/piste.dtd";
    QDomDocument doc(impl.createDocumentType(name,publicId,systemId));

    // add some XML comment at the beginning
    doc.appendChild(doc.createComment("This file describe a gamel"));
    doc.appendChild(doc.createTextNode("\n")); // for nicer output

    // root node
    QDomElement pNode = doc.createElement("piste");
    pNode.setAttribute("name",fichier);
    doc.appendChild(pNode);

    for (unsigned int i=0; i<tab.size(); i++) tab[i]->saveXML(doc, pNode);

    QFile f(fichier);
    f.open(QIODevice::WriteOnly);
    QTextStream out(&f);
    out << doc.toString();
    f.close();
}

double Piste::getScotchDistance(double x, double y)
{
    double d0=1e20, d1;

    for (unsigned int i=0; i<tab.size(); i++) {
        d1 = tab[i]->getScotchDistance(x, y);
        if (d1 >= 0) if (d1 < d0) d0 = d1;
    }
    return d0;
}

bool Piste::getObstacleCollision(double x, double y)
{
    for (unsigned int i=0; i<tab.size(); i++) {
        if (tab[i]->getObstacleCollision(x, y)) return true;
    }
    return false;
}

void Piste::setIDs()
{
    for (unsigned int i = 0; i<tab.size(); i++) tab[i]->setID(i);
}

Portion *Piste::eltCreate(int t)
{
    Portion *p = NULL;

    if (t == PortionRepere::TypeIndex()) {
        p = new PortionRepere();
    } else if (t == PortionLigneDroite::TypeIndex()) {
        p = new PortionLigneDroite();
    } else if (t == PortionLigneCourbe::TypeIndex()) {
        p = new PortionLigneCourbe();
    } else if (t == PortionObstacle::TypeIndex()) {
        p = new PortionObstacle();
    } else if (t == PortionLigneAveugle::TypeIndex()) {
        p = new PortionLigneAveugle();
    } else if (t == PortionMarqueurGauche::TypeIndex()) {
        p = new PortionMarqueurGauche();
    }
    return p;
}

void Piste::eltChange(unsigned int n)
{
    for (unsigned int i = n+1; i<tab.size(); i++) {
        tab[i]->calculPosition();
    }
}

void Piste::eltSupprime(unsigned int n)
{
    if (n>=tab.size()) return;
    delete tab[n];
    tab.erase(tab.begin()+n);
    Portion *p = NULL;
    if (n>0) p = tab[n-1];
    if (n<tab.size()) tab[n]->setSrc(p);
    eltChange(n);
    setIDs();
}

void Piste::eltRemplace(unsigned int n, int t)
{
    if (n>=tab.size()) return;
    delete tab[n];
    tab[n] = eltCreate(t);
    Portion *p = NULL;
    if (n>0) p = tab[n-1];
    tab[n]->setSrc(p);
    if (n+1<tab.size()) tab[n+1]->setSrc(tab[n]);
    eltChange(n);
    setIDs();
    if (scene) tab[n]->ajouterDansScene(*scene);
}

void Piste::eltInsert(unsigned int n, int t)
{
    if (n>tab.size()) return;
    tab.insert(tab.begin()+n, eltCreate(t));
    Portion *p = NULL;
    if (n>0) p = tab[n-1];
    tab[n]->setSrc(p);
    if (n+1<tab.size()) tab[n+1]->setSrc(tab[n]);
    eltChange(n);
    setIDs();
    if (scene) tab[n]->ajouterDansScene(*scene);
}

void Piste::eltModifArg(unsigned int n, int arg, double val)
{
    if (n>=tab.size()) return;
    tab[n]->modifArg(arg, val);
    eltChange(n);
}

void Piste::setFocus(unsigned int n)
{
    if (n>=tab.size()) return;
    if (focus<tab.size()) tab[focus]->setFocus(false);
    tab[n]->setFocus();
    focus = n;
}

void Piste::noFocus()
{
    if (focus<tab.size()) tab[focus]->setFocus(false);
    focus = -1;
}
