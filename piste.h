#ifndef PISTE_H
#define PISTE_H

#include <math.h>
#include "xmlopensavedata.h"
#include <vector>

#include <QGraphicsScene>
#include <QGraphicsItem>

class Portion {
protected :
    double x0, y0, theta0;
    double cosTheta0, sinTheta0; // memo du cos et du sin pour accelerer les calculs
    bool noSrc, focus, itemCalc;
    int id;
    Portion *source;
    QGraphicsItem *item, *itemFocus;
    QGraphicsScene *scene;

public :
    Portion(int n=-1, Portion *src=NULL);
    virtual ~Portion();
    void calculPosition();
    void setSrc(Portion *src) {  source = src;  calculPosition();  }
    void ajouterDansScene(QGraphicsScene &s);
    void enleverDeScene() {
        if (item) if (scene) scene->removeItem(item);
        if (itemFocus) if (scene) scene->removeItem(itemFocus);
        scene = NULL;
    }
    QGraphicsScene *getScene() {  return scene;  }
    virtual double getX() {  return x0;  }
    virtual double getY() {  return y0;  }
    virtual double getTheta() {  return theta0;  }
    int getID() {  return id;  }
    void setID(int n) {  id = n;  }
    virtual void saveXML(QDomDocument &doc, QDomElement &pNode) = 0;
    virtual double getScotchDistance(double /* x */, double /* y */) {  return -1;  }; // valeur negative -> infini : pas de scotch
    virtual bool getObstacleCollision(double /* x */, double /* y */) {  return false;  } // pas d'obstacle

    virtual int typeIndex() = 0;
    virtual int nbArg() {  return 0;  }
    virtual QString nameArg(int /* n */) {  return QString("");  }
    virtual double valArg(int /* n */) {  return 0.0;  }
    virtual double maxArg(int /* n */) {  return 0.0;  }
    virtual double minArg(int /* n */) {  return 0.0;  }
    virtual void modifArg(int /* n */, double /* val */) {}
    virtual QString suffixArg(int /* n */) {  return QString("");  }
    virtual int decimalsArg(int /* n */) {  return 1;  }

    void setFocus(bool oui = true) {  focus = oui;  if (itemFocus) itemFocus->setVisible(oui);  }
    virtual void calculItem() {  itemCalc = true;  }
    void moveItem();
};

class PortionRepere : public Portion
{
public :
    void setTheta(double theta) {  theta0 = theta; cosTheta0 = cos(theta0); sinTheta0 = sin(theta0);  calculItem();  }
    PortionRepere(int n=-1, double x=0, double y=0, double theta=0) : Portion(n) {  noSrc = true;  x0=x;  y0=y;  setTheta(theta);  }
    void setX(double x) {  x0 = x;  calculItem();  }
    void setY(double y) {  y0 = y;  calculItem();  }
    void setThetaDeg(double theta) {  setTheta(theta*(M_PI/180.0));  calculItem();  }
    virtual void saveXML(QDomDocument &doc, QDomElement &pNode);

    virtual int typeIndex() {  return 0;  }
    static int TypeIndex() {  return 0;  }
    virtual int nbArg() {  return 3;  }
    virtual QString nameArg(int n);
    virtual double valArg(int n);
    virtual double maxArg(int n);
    virtual double minArg(int n);
    virtual void modifArg(int n, double val);
    virtual QString suffixArg(int n) {  if (n<2) return QString(" mm"); else return QString("°"); }
    virtual int decimalsArg(int n) {  if (n<2) return 0; else return 1; }

    virtual void calculItem();
};

class PortionLigneDroite : public Portion
{
protected :
    double longueur, largeur;

public :
    PortionLigneDroite(int n=-1, double l=1000, double scotch=19, Portion *src=NULL) : Portion(n, src) {  longueur=l; largeur=scotch;  }
    virtual double getX() {  return x0 + longueur * cos(theta0);  }
    virtual double getY() {  return y0 + longueur * sin(theta0);  }
    void setLongueur(double l) {  longueur = l;  calculItem();  }
    void setLargeur(double l) {  largeur = l;  calculItem();  }
    virtual void saveXML(QDomDocument &doc, QDomElement &pNode);
    virtual double getScotchDistance(double x, double y);
    double getLongueur() {  return longueur;  }

    virtual int typeIndex() {  return 1;  }
    static int TypeIndex() {  return 1;  }
    virtual int nbArg() {  return 1;  }
    virtual QString nameArg(int n) {  if (n==0) return QString("longueur"); else return QString("rien");  }
    virtual double valArg(int n) {  if (n==0) return longueur; else return 0.0;  }
    virtual double maxArg(int n) {  if (n==0) return 10000.0; else return 0.0;  }
    virtual double minArg(int n) {  if (n==0) return 0.0; else return 0.0;  }
    virtual void modifArg(int n, double val) {  if (n==0) longueur = val;  calculItem();  }
    virtual QString suffixArg(int /* n */) {  return QString(" mm");  }
    virtual int decimalsArg(int /* n */) {  return 0;  }

    virtual void calculItem();
};

class PortionLigneAveugle : public Portion
{
protected :
    double longueur;

public :
    PortionLigneAveugle(int n=-1, double l=1000, Portion *src=NULL) : Portion(n, src) {  longueur=l;  }
    virtual double getX() {  return x0 + longueur * cos(theta0);  }
    virtual double getY() {  return y0 + longueur * sin(theta0);  }
    void setLongueur(double l) {  longueur = l;  calculItem();  }
    virtual void saveXML(QDomDocument &doc, QDomElement &pNode);
    double getLongueur() {  return longueur;  }

    virtual int typeIndex() {  return 4;  }
    static int TypeIndex() {  return 4;  }
    virtual int nbArg() {  return 1;  }
    virtual QString nameArg(int n) {  if (n==0) return QString("longueur"); else return QString("rien");  }
    virtual double valArg(int n) {  if (n==0) return longueur; else return 0.0;  }
    virtual double maxArg(int n) {  if (n==0) return 10000.0; else return 0.0;  }
    virtual double minArg(int n) {  if (n==0) return 0.0; else return 0.0;  }
    virtual void modifArg(int n, double val) {  if (n==0) longueur = val;  calculItem();  }
    virtual QString suffixArg(int /* n */) {  return QString(" mm");  }
    virtual int decimalsArg(int /* n */) {  return 0;  }

    virtual void calculItem();
};

class PortionLigneCourbe : public Portion
{
protected :
    double rayon, largeur, angle;

public :
    PortionLigneCourbe(int n=-1, double r=1000, double a=M_PI/2, double scotch=19, Portion *src=NULL) : Portion(n, src) {  rayon=r; angle = a; largeur=scotch;  }
    virtual double getX();
    virtual double getY();
    virtual double getTheta() {  return theta0 + angle;  }
    void setRayon(double r) {  rayon = r;  calculItem();  }
    void setLargeur(double l) {  largeur = l;  calculItem();  }
    void setAngle(double a) {  angle = a;  calculItem();  }
    void setAngleDeg(double a) {  angle = a*(M_PI/180.0);  calculItem();  }
    virtual void saveXML(QDomDocument &doc, QDomElement &pNode);
    virtual double getScotchDistance(double x, double y);
    double getRayon() {  return rayon;  }
    double getAngleDeg() {  return angle * (180.0/M_PI);  }

    virtual int typeIndex() {  return 2;  }
    static int TypeIndex() {  return 2;  }
    virtual int nbArg() {  return 2;  }
    virtual QString nameArg(int n);
    virtual double valArg(int n);
    virtual double maxArg(int n);
    virtual double minArg(int n);
    virtual void modifArg(int n, double val);
    virtual QString suffixArg(int n) {  if (n==0) return QString(" mm"); else return QString("°"); }
    virtual int decimalsArg(int n) {  if (n==0) return 0; else return 1; }

    virtual void calculItem();
};

class PortionObstacle : public Portion
{
protected :
    double longueur, largeur;

public :
    PortionObstacle(int n=-1, double l=1000, double bois=50, Portion *src=NULL) : Portion(n, src) {  longueur = l; largeur = bois;  }
    void setLongueur(double l) {  longueur = l;  calculItem();  }
    void setLargeur(double l) {  largeur = l;  calculItem();  }
    virtual void saveXML(QDomDocument &doc, QDomElement &pNode);
    virtual bool getObstacleCollision(double x, double y);
    double getLongueur() {  return longueur;  }
    double getLargeur() {  return largeur;  }

    virtual int typeIndex() {  return 3;  }
    static int TypeIndex() {  return 3;  }
    virtual int nbArg() {  return 2;  }
    virtual QString nameArg(int n);
    virtual double valArg(int n);
    virtual double maxArg(int n);
    virtual double minArg(int n);
    virtual void modifArg(int n, double val);
    virtual QString suffixArg(int /* n */) {  return QString(" mm");  }
    virtual int decimalsArg(int /* n */) {  return 0;  }

    virtual void calculItem();
};

class PortionMarqueurGauche : public Portion
{
protected :
    double longueur, largeur;

public :
    PortionMarqueurGauche(int n=-1, double l=300, double scotch=19, Portion *src=NULL) : Portion(n, src) {  longueur=l; largeur=scotch;  }
    void setLongueur(double l) {  longueur = l;  calculItem();  }
    void setLargeur(double l) {  largeur = l;  calculItem();  }
    virtual void saveXML(QDomDocument &doc, QDomElement &pNode);
    virtual double getScotchDistance(double x, double y);
    double getLongueur() {  return longueur;  }

    virtual int typeIndex() {  return 5;  }
    static int TypeIndex() {  return 5;  }
    virtual int nbArg() {  return 1;  }
    virtual QString nameArg(int n) {  if (n==0) return QString("longueur"); else return QString("rien");  }
    virtual double valArg(int n) {  if (n==0) return longueur; else return 0.0;  }
    virtual double maxArg(int n) {  if (n==0) return 10000.0; else return 0.0;  }
    virtual double minArg(int n) {  if (n==0) return 0.0; else return 0.0;  }
    virtual void modifArg(int n, double val) {  if (n==0) longueur = val;  calculItem();  }
    virtual QString suffixArg(int /* n */) {  return QString(" mm");  }
    virtual int decimalsArg(int /* n */) {  return 0;  }

    virtual void calculItem();
};

class Piste : public XMLOpenSaveData
{
    Q_OBJECT

protected :
    std::vector<Portion *> tab;
    unsigned int focus;

    Portion *eltCreate(int t);
    void setIDs();

    QGraphicsScene *scene;

public:
    Piste();
    ~Piste();
    void ajoute(Portion &p) {  if (tab.size()>0) p.setSrc(tab.back());  tab.push_back(&p);  }
    void ajouterDansScene(QGraphicsScene &s);
    virtual void initialise();
    virtual void decode(const QString &obj, const QString &num, const QString &attr, const QString &val);
    template <typename T> T &getPortion(int n);
    void saveData(const QString &fichier);
    double getScotchDistance(double x, double y);
    bool getObstacleCollision(double x, double y);
    unsigned int size() {  return tab.size();  }
    Portion *getElt(unsigned int n) {  return tab[n];  }

public slots :
    void eltChange(unsigned int n);
    void eltSupprime(unsigned int n);
    void eltRemplace(unsigned int n, int t);
    void eltInsert(unsigned int n, int t);
    void eltModifArg(unsigned int n, int arg, double val);
    void setFocus(unsigned int n);
    void noFocus();
};

#endif // PISTE_H
