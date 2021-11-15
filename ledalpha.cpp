#include "ledalpha.h"

#include <QPixmap>

LedAlpha::LedAlpha(QWidget *parent) :
    QGraphicsView(parent)
{
    setInteractive(false);
    setStyleSheet("LedAlpha {background : transparent;}");
    on.setPixmap(QPixmap(":/images/led_verte_on.png"));
    on.setPos(0,0);
    on.setScale(1);
    on.setOpacity(0);
    gs.addItem(&on);
    off.setPixmap(QPixmap(":/images/led_verte_off.png"));
    off.setPos(0,0);
    off.setScale(1);
    off.setOpacity(1.0);
    gs.addItem(&off);
    setScene(&gs);
    courantMax = 20e-3;
    courant = 0;
}
