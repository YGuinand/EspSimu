#ifndef OSCILLO_H
#define OSCILLO_H

#include "processeur.h"
#include "evenement.h"
#include "ports.h"

#include <QChartView>
#include <QLineSeries>
#include <QPointF>
#include <QValueAxis>

#define NB_PTS  250

class Oscillo : public Evenement
{
private :
    QChart *figure;
    QLineSeries *courbe;
    QValueAxis *axisX, *axisY;
    Connexion *observ;

    int dt;
    P18f4550 *processeur;

    double ymem[NB_PTS], xmem[NB_PTS], trigNiveau, valActu;
    QList<QPointF> pts;

    uint16_t index, trigTemps;
    int trigAuto, nbAcq;
    int trigEtat;

    int numParam;

public:
    Oscillo(P18f4550 *p, QChartView *qp);
    virtual ~Oscillo() {}

    virtual void executer();
    void observer(Connexion *c, int param);
    void debut();

    void setBaseTemps(int d);
};

#endif // OSCILLO_H
