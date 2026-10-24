#include <cmath>

#include "oscillo.h"
#include "connexion.h"

#include <QPen>
#include <QAbstractAxis>
#include <QList>
#include <QGraphicsLayout>

Oscillo::Oscillo(QChartView *qp)
{
    figure = new QChart();
    qp->setChart(figure);
    figure->setMargins(QMargins(0,0,0,0));

    trigTemps = 1;
    unite = 1;

    courbe = new QLineSeries();
    figure->addSeries(courbe);
    figure->legend()->setVisible(false);

    axisY = new QValueAxis;
    axisY->setTickType(QValueAxis::TicksDynamic);
    axisY->setTickAnchor(0);
    axisY->setTickInterval(1);
    axisY->setLabelFormat("%.0f");
    axisY->setRange(-0.1, 3.5);
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

    setBaseTemps(29);
}

void Oscillo::setBaseTemps(int d)
{
    dt = exp(0.085619911*d);

    unite = 1;
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

    rafraichir();
}


// Niveau du signal PWM a l'echantillon i : front montant a l'echantillon trigTemps
double Oscillo::tension(int i, double rapport, double frequence) const
{
    if (rapport <= 0.001) return 0.0;
    if (rapport >= 0.999) return VCC_CARTE;
    double t = (i - trigTemps) * dt / 48e6;
    double phase = t * frequence;
    phase -= floor(phase);
    return (phase < rapport) ? VCC_CARTE : 0.0;
}

void Oscillo::rafraichir()
{
    double rapport = 0.0, frequence = 1000.0;
    if (source) source(rapport, frequence);

    QList<QPointF> pts;
    double actuel = tension(0, rapport, frequence);
    pts.append(QPointF(xmem[0], actuel));
    for (int i=1; i<NB_PTS; i++) {
        double v = tension(i, rapport, frequence);
        if (v != actuel) {
            pts.append(QPointF(xmem[i], actuel));
            actuel = v;
            pts.append(QPointF(xmem[i], actuel));
        }
    }
    pts.append(QPointF(xmem[NB_PTS-1], actuel));
    courbe->replace(pts);
}
