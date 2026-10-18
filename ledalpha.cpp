#include "ledalpha.h"

#include <QPixmap>

LedAlpha::LedAlpha(QWidget *parent) :
    QGraphicsView(parent)
{
    setInteractive(false);
    setStyleSheet("LedAlpha {background : transparent;}");

    on.setPixmap(QPixmap(":/images/led_verte_on.png"));
    off.setPixmap(QPixmap(":/images/led_verte_off.png"));

    on.setPos(0,0);
    on.setScale(1);
    on.setOpacity(0);
    gs.addItem(&on);
    off.setPos(0,0);
    off.setScale(1);
    off.setOpacity(1.0);
    gs.addItem(&off);
    setScene(&gs);
    courantMax = 20e-3;
    courant = 0;
}

void LedAlpha::setColor(Couleur c)
{
    switch (c) {
    case JAUNE:
        on.setPixmap(QPixmap(":/images/led_jaune_on.png"));
        off.setPixmap(QPixmap(":/images/led_jaune_off.png"));
        break;
    case ROUGE:
        on.setPixmap(QPixmap(":/images/led_rouge_on.png"));
        off.setPixmap(QPixmap(":/images/led_rouge_off.png"));
        break;
    default: // VERT
        on.setPixmap(QPixmap(":/images/led_verte_on.png"));
        off.setPixmap(QPixmap(":/images/led_verte_off.png"));
        break;
    }
}
