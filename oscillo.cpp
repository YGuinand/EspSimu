#include <cstring>

#include "oscillo.h"

Oscillo::Oscillo(P18f4550 *p, QwtPlot *qp)
{
    processeur = p;
    qwtPlot = qp;

    trigTemps = 1;

    for (int i=0; i<NB_PTS; i++) {
        x[i]=(i-trigTemps)/(48e6/dt)*1e6;
        y[i]=0;
    }

    qwtPlot->setAxisScale(QwtPlot::yLeft, 0, 5);
    qwtPlot->setAxisTitle(QwtPlot::yLeft, QString("Volts"));

    courbe.setRawSamples(x, y, NB_PTS);
    courbe.attach(qwtPlot);
    courbe.setStyle(QwtPlotCurve::Steps);
    courbe.setYAxis(QwtPlot::yLeft);

    QPen pen(Qt::DotLine);
    pen.setColor(QColor(255,255,255));
    grille.setMajorPen(pen);
    grille.attach(qwtPlot);

    trigAuto = 5000;
    trigNiveau = 2.5;
    trigEtat = 0;
    nbAcq = 0;

    numParam = 0;

    setBaseTemps(29);
}

void Oscillo::setBaseTemps(int d)
{
    if (estEnregistre()) supprimer();
    dt = exp(0.085619911*d);
    double unite = 1;
    double bx = NB_PTS/(48e6/dt);
    QString str("s");
    if (bx<1e-3) {
        unite = 1e6;
        str = QString(0x03bc)+QString("s");
    } else if (bx<1) {
        unite = 1e3;
        str = "ms";
    }
    for (int i=0; i<NB_PTS; i++) {
        x[i]=(i-trigTemps)/(48e6/dt)*unite;
    }
    qwtPlot->setAxisScale(QwtPlot::xBottom, x[0], x[NB_PTS-1]);
    qwtPlot->setAxisTitle(QwtPlot::xBottom, str);
    qwtPlot->replot();
}

void Oscillo::observer(Connexion *c, int param)
{
    observ = c;
    numParam = param;
}

void Oscillo::debut()
{
    trigEtat = 0;
    nbAcq = 0;
    index = 0;
    if (estEnregistre()) supprimer();
    if (observ) enregistrerDernier(dt, processeur->getListeEvts());
}

void Oscillo::executer()
{
    uint16_t i, j;

    if (nbAcq > trigAuto) {
        i=0;
        j=index;
        do {
            if (j>=NB_PTS) j=0;
            y[i++] = ymem[j++];
        } while (i < NB_PTS);
        qwtPlot->replot();
        supprimer();
        return;
    }
    nbAcq++;
    supprimer();
    if (trigEtat==0) {
        ymem[index++] = observ->getValParam(numParam);
        if (index>=NB_PTS) {
            trigEtat=1;
            nbAcq = 0;
            index = 0;
        }
        enregistrerDernier(dt, processeur->getListeEvts());
    } else if (trigEtat==1) {
        ymem[index] = observ->getValParam(numParam);
        if (ymem[index++]<trigNiveau) {
            trigEtat=2;
            nbAcq = 0;
        }
        if (index>=NB_PTS) index=0;
        enregistrerDernier(dt, processeur->getListeEvts());
    } else if (trigEtat==2) {
        ymem[index] = observ->getValParam(numParam);
        if (ymem[index]>trigNiveau) {
            trigEtat=3;
            nbAcq = 0;
            j = index - trigTemps;
            if (j>=NB_PTS) j+=NB_PTS;
            for (i=0; i<=trigTemps; i++, j++) {
                if (j>=NB_PTS) j = 0;
                y[i] = ymem[j];
            }
            index = trigTemps;
        }
        index++;
        if (index>=NB_PTS) index=0;
        enregistrerDernier(1, processeur->getListeEvts());
    } else {
        y[index++] = observ->getValParam(numParam);
        if (index<NB_PTS) {
            enregistrerDernier(dt, processeur->getListeEvts());
        } else {
            qwtPlot->replot();
        }
    }

}
