#ifndef FENETREOSCILLOS_H
#define FENETREOSCILLOS_H

#include <QFrame>
#include <QCloseEvent>

#include "modele.h"
#include "oscillo.h"

namespace Ui {
    class FenetreOscillos;
}

class LedPwmCNX : public Connexion {
private :
    QGraphicsScene gs;
    QGraphicsPixmapItem on, off;
    uint64_t temps;
    double vmemo;
    double vs, alpha, alphaMemo;
    P18f4550 *processeur;
public :
    LedPwmCNX() { processeur=NULL; setSrc(0.7, 380);  vs=0.0;  vmemo=0.0;  temps=0;  alphaMemo = 2.0; }
    void setGraphics(P18f4550 *p, QGraphicsView *g, QString str_on, QString str_off);
    virtual void modifie(double v);
    void reset() {  vs=0;  vmemo=0;  alpha=0;  redraw();  }
    void redraw() {  if (fabs(alpha-alphaMemo)>0.05) {  alphaMemo = alpha;  on.setOpacity(alpha); off.setOpacity(1-alpha);  } }
};

class FenetreOscillos : public QFrame
{
    Q_OBJECT

public:
    explicit FenetreOscillos(Modele &m, QWidget *parent = 0);
    ~FenetreOscillos();

private:
    Ui::FenetreOscillos *ui;

    Modele *modele;
    Oscillo *oscillo1, *oscillo2;

    LedPwmCNX ledPwm1, ledPwm2;

private slots:
    void on_baseTpsOscillo1_valueChanged(int value);
    void on_baseTpsOscillo2_valueChanged(int value);
    void on_checkBoxBaseTps2_clicked(bool checked);

signals :
    void estCache(bool);

public slots :
    void miseAJour() {  oscillo1->debut(); oscillo2->debut();  }
    void cnxChange();

protected :
    virtual void closeEvent (QCloseEvent* event) {  event->ignore();  if (parentWidget()) parentWidget()->hide(); else hide(); emit estCache(true); }

};

#endif // FENETREOSCILLOS_H
