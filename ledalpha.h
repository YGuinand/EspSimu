#ifndef LEDALPHA_H
#define LEDALPHA_H

#include <math.h>

#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>

class LedAlpha : public QGraphicsView
{
    Q_OBJECT

private :
    QGraphicsScene gs;
    QGraphicsPixmapItem on, off;
    double courantMax, courant, variation;

public:
    explicit LedAlpha(QWidget *parent = 0);

public slots:
    void setCourant(double i) {
        if (i>courantMax) i=courantMax;
        if (fabs(i-courant)>variation) {
            courant=i;
            double alpha=i/courantMax;
            on.setOpacity(alpha);
            off.setOpacity(1-alpha);
        }
    }
    void setCourantMax(double i) { courantMax = i;  double alpha=courant/courantMax; on.setOpacity(alpha); off.setOpacity(1-alpha); }

};

#endif // LEDALPHA_H
