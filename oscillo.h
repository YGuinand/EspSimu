#ifndef OSCILLO_H
#define OSCILLO_H

#include "processeur.h"
#include "evenement.h"
#include "ports.h"

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>

#define NB_PTS  250

class Oscillo : public Evenement
{
private :
    QwtPlot *qwtPlot;
    QwtPlotCurve courbe;
    QwtPlotGrid grille;
    Connexion *observ;

    int dt;
    P18f4550 *processeur;

    double x[NB_PTS], y[NB_PTS], ymem[NB_PTS], trigNiveau;
    uint16_t index, trigTemps;
    int trigAuto, nbAcq;
    int trigEtat;

    int numParam;

public:
    Oscillo(P18f4550 *p, QwtPlot *qp);
    virtual ~Oscillo() {}

    virtual void executer();
    void observer(Connexion *c, int param);
    void debut();

    void setBaseTemps(int d);
};

#endif // OSCILLO_H
