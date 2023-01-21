#include <cstring>

#include "oscillo.h"

#include <QPen>
#include <QAbstractAxis>
#include <QList>
#include <QGraphicsLayout>

Oscillo::Oscillo(P18f4550 *p, QChartView *qp)
{
    processeur = p;

    figure = new QChart();
    qp->setChart(figure);
    figure->setMargins(QMargins(0,0,0,0));

    trigTemps = 1;

    courbe = new QLineSeries();
    figure->addSeries(courbe);
    figure->legend()->setVisible(false);

    axisY = new QValueAxis;
    axisY->setTickType(QValueAxis::TicksDynamic);
    axisY->setTickAnchor(0);
    axisY->setTickInterval(1);
    axisY->setTickCount(6);
    axisY->setLabelFormat("%.0f");
    axisY->setRange(-0.1, 5.1);
    axisY->setTitleText("Volts");
    figure->addAxis(axisY, Qt::AlignLeft);
    courbe->attachAxis(axisY);

    axisX = new QValueAxis;
    axisX->setTickType(QValueAxis::TicksDynamic);
    axisX->setTickAnchor(0);
    axisX->setTickInterval(1);
    axisX->setLabelFormat("%.0f");
    axisX->setRange(-0.1, 5.1);
    axisX->setTitleText("s");
    figure->addAxis(axisX, Qt::AlignBottom);
    courbe->attachAxis(axisX);

    trigAuto = 50000;
    trigNiveau = 2.6;
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
        str = QChar(0x03bc)+QString("s");
    } else if (bx<1) {
        unite = 1e3;
        str = "ms";
    }

    double delta = 1.0/(48e6/dt)*unite;
    for (int i=0; i<NB_PTS; i++) {
        xmem[i] = (i-trigTemps)*delta;
    }

    axisX->setLabelFormat("%.0f");
    double ecart = xmem[NB_PTS-1]-xmem[0];

    trigAuto = 0.1*unite/ecart*NB_PTS;

    if (ecart>800) ecart = 200;
    else if (ecart>400) ecart = 100;
    else if (ecart>200) ecart = 50;
    else if (ecart>80) ecart = 20;
    else if (ecart>40) ecart = 10;
    else if (ecart>20) ecart = 5;
    else if (ecart>8) ecart = 2;
    else if (ecart>4) ecart = 1;
    else {
        axisX->setLabelFormat("%.1f");
        if (ecart>2) ecart = 0.5;
        else if (ecart>0.8) ecart = 0.2;
        else ecart = 0.1;
    }

    axisX->setTickInterval(ecart);
    axisX->setRange(xmem[0], xmem[NB_PTS-1]);
    axisX->setTitleText(str);

    pts.clear();
    courbe->replace(pts);
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
        j=index;
        valActu = ymem[j++];
        pts.clear();
        pts.append(QPointF(xmem[0], valActu));
        for (i=1; i<NB_PTS; i++, j++)
        {
            if (j>=NB_PTS) j=0;
            if (ymem[j] != valActu) {
                pts.append(QPointF(xmem[i], valActu));
                valActu = ymem[j];
                pts.append(QPointF(xmem[i], valActu));
            }
        }
        pts.append(QPointF(xmem[NB_PTS-1], valActu));
        courbe->replace(pts);
        trigEtat = 0;
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
            j = index - trigTemps;      // si dépassement j >= NB_PTS car j non-signé
            if (j>=NB_PTS) j+=NB_PTS;   // on élimine le dépassement
            valActu = ymem[j++];
            pts.clear();
            pts.append(QPointF(xmem[0], valActu));
            for (i=1; i<=trigTemps; i++, j++)
            {
                if (j>=NB_PTS) j=0;
                if (ymem[j] != valActu) {
                    pts.append(QPointF(xmem[i], valActu));
                    valActu = ymem[j];
                    pts.append(QPointF(xmem[i], valActu));
                }
            }
            index = trigTemps;
        }
        index++;
        if (index>=NB_PTS) index=0;
        enregistrerDernier(dt, processeur->getListeEvts());
    } else {
        double val = observ->getValParam(numParam);
        if (val != valActu) {
            pts.append(QPointF(xmem[index], valActu));
            valActu = val;
            pts.append(QPointF(xmem[index], valActu));
        }
        index++;
        if (index<NB_PTS) {
            enregistrerDernier(dt, processeur->getListeEvts());
        } else {
            pts.append(QPointF(xmem[NB_PTS-1], valActu));
            courbe->replace(pts);
        }
    }
}
