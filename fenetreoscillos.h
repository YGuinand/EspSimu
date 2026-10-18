#ifndef FENETREOSCILLOS_H
#define FENETREOSCILLOS_H

#include <QFrame>
#include <QCloseEvent>
#include <QTimer>
#include <functional>

#include "modele.h"
#include "oscillo.h"

namespace Ui {
    class FenetreOscillos;
}

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
    QTimer *timer;

    // Sources de signal pour les deux oscilloscopes (GPIO brute)
    std::function<bool(double &, double &)> srcGpio2, srcGpio4, srcGpio26, srcGpio27;

private slots:
    void on_baseTpsOscillo1_valueChanged(int value);
    void on_baseTpsOscillo2_valueChanged(int value);
    void on_checkBoxBaseTps2_clicked(bool checked);

signals :
    void estCache(bool);

public slots :
    void miseAJour();

protected :
    virtual void closeEvent (QCloseEvent* event) {  event->ignore();  if (parentWidget()) parentWidget()->hide(); else hide(); emit estCache(true); }

};

#endif // FENETREOSCILLOS_H
