#ifndef FENETREPISTE_H
#define FENETREPISTE_H

#include <QFrame>
#include <QCloseEvent>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QLabel>
#include <QDoubleSpinBox>

#include "modele.h"

namespace Ui {
    class FenetrePiste;
}

class GamelItem : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT

protected :
    double xMax, xMin, yMax, yMin;

signals :
    void moved(qreal, qreal, qreal);

public :
    GamelItem(double xmin, double ymin, double xmax, double ymax) : QObject(), QGraphicsPixmapItem(QPixmap(":/images/move_gamel.png")) {
        xMax = xmax; yMax = ymax; xMin = xmin; yMin = ymin;
        setFlag(QGraphicsItem::ItemIsMovable);
        setFlag( QGraphicsItem::ItemSendsGeometryChanges);
        setOffset(-32,-32);
        setScale(240.0/64.0);
    }
    void signalMoved() {  emit moved(x(),-y(),-rotation());  }

protected :
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) {
        if (change == ItemPositionChange && scene()) {
            QPointF newPos = value.toPointF();
            if (newPos.x()>xMax) newPos.setX(xMax); else if (newPos.x()<xMin) newPos.setX(xMin);
            if (newPos.y()>yMax) newPos.setY(yMax); else if (newPos.y()<yMin) newPos.setY(yMin);
            return newPos;
        } else if (change == ItemPositionHasChanged && scene()) emit moved(x(),-y(),-rotation());
        return QGraphicsPixmapItem::itemChange(change, value);
    }

public slots :
    void deplacement(double xn, double yn, double thetaRad) {
        setFlag( QGraphicsItem::ItemSendsGeometryChanges, false);
        yn = -yn;
        if (xn > xMax) xn = xMax; else if (xn < xMin) xn = xMin;
        if (yn > yMax) yn = yMax; else if (yn < yMin) yn = yMin;
        setPos(xn, yn);
        setRotation(-thetaRad*(180.0/M_PI));
        setFlag( QGraphicsItem::ItemSendsGeometryChanges);
    }
};

class FenetrePiste : public QFrame
{
    Q_OBJECT

public:
    explicit FenetrePiste(Modele &m, QWidget *parent = 0);
    ~FenetrePiste();

    // Fichier d'initialisation (voir parametres.h) : au demarrage, charger la
    // derniere piste utilisee si elle existe encore.
    void chargePisteRecent();

private:
    Ui::FenetrePiste *ui;

    Modele *modele;

    QGraphicsScene scene;
    GamelItem *gamel;

    QString fichier;

    void drawPiste();
    void editPiste();

    unsigned int numEdit;
    QLabel *label[3];
    QDoubleSpinBox *dbl[3];

signals :
    void estCache(bool);

protected :
    virtual void closeEvent (QCloseEvent* event) {  event->ignore();  if (parentWidget()) parentWidget()->hide(); else hide();  emit estCache(true); }

private slots:
    void on_toolButtonZoomP_clicked();
    void on_toolButtonZoomM_clicked();
    void on_toolButtonClk_clicked();
    void on_toolButtonAntiClk_clicked();
    void on_toolButtonPoseGamel_toggled(bool checked);
    void on_toolButtonNext_clicked();
    void on_toolButtonPrev_clicked();
    void on_toolButtonNew_clicked();
    void on_toolButtonSuppr_clicked();
    void on_comboBoxTypes_currentIndexChanged(int index);
    void on_doubleSpinBox1_valueChanged(double val);
    void on_doubleSpinBox2_valueChanged(double val);
    void on_doubleSpinBox3_valueChanged(double val);
    void on_toolButtonOpen_clicked();
    void on_toolButtonSave_clicked();
    void on_toolButtonEdit_toggled(bool checked);
    void on_toolButtonZoomFit_clicked();
};

#endif // FENETREPISTE_H
