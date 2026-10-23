#ifndef GAMEL_H
#define GAMEL_H

#include <QFrame>
#include <QCloseEvent>
#include <QComboBox>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QCheckBox>
#include <QPushButton>

#include "modele.h"

namespace Ui {
    class Gamel;
}

class FocusItemCNY : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT

protected :
    double xMax, xMin, yMax, yMin;

public :
    FocusItemCNY(double xmin, double ymin, double xmax, double ymax) : QObject(), QGraphicsPixmapItem(QPixmap(":/images/cny70focus.png")) {
        xMax = xmax; yMax = ymax; xMin = xmin; yMin = ymin;
        setFlag(QGraphicsItem::ItemIsMovable);
        setFlag( QGraphicsItem::ItemSendsGeometryChanges);
    }

signals :
    void moved();

protected :
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) {
        if (change == ItemPositionChange && scene()) {
            QPointF newPos = value.toPointF();
            if (newPos.x()>xMax) newPos.setX(xMax);
            else if (newPos.x()<xMin) newPos.setX(xMin);
            if (newPos.y()>yMax) newPos.setY(yMax);
            else if (newPos.y()<yMin) newPos.setY(yMin);
            return newPos;
        } else if (change == ItemPositionHasChanged && scene()) emit moved();
        return QGraphicsPixmapItem::itemChange(change, value);
    }
};

class Gamel : public QFrame
{
    Q_OBJECT

public:
    explicit Gamel(Modele &m, QWidget *parent = 0);
    ~Gamel();

    // Fichier d'initialisation (voir parametres.h) : au demarrage, charger la
    // derniere gamelle utilisee si elle existe encore.
    void chargeGamelleRecent();

private:
    Ui::Gamel *ui;

    QGraphicsScene scene;
    QGraphicsPixmapItem *tabItems[Modele::NB_MAX_CAPT_IR];
    FocusItemCNY *focusItem;
    const double echelle;
    const double delta;

    QCheckBox *fdc;

    Modele *modele;

    QString fichier;

    bool pwm1aGauche;

    int captIrIndex;

    void changerJackSchema();
    void changerFdcSchema();
    void changerCaptIrSchema();

    void changePinsInUse();

    void remplirComboJ2(QComboBox &c, int indexCourant);

    double changeRepX(double x) { return x*echelle-delta; }
    double changeRepY(double y) { return -y*echelle-delta; }
    double changeInvRepX(double px) { return (px+delta)/echelle; }
    double changeInvRepY(double py) { return -(py+delta)/echelle; }

signals :
    void estCache(bool);

protected :
    virtual void closeEvent (QCloseEvent* event) {  event->ignore();  if (parentWidget()) parentWidget()->hide(); else hide();  emit estCache(true); }

private slots:
    void on_jackContact_currentIndexChanged(int );
    void on_jackSchema_currentIndexChanged(int );
    void on_jackPin_currentIndexChanged(int index);
    void on_jackResistance_valueChanged(double );
    void on_fdcPin_currentIndexChanged(int index);
    void on_fdcContact_currentIndexChanged(int );
    void on_fdcSchema_currentIndexChanged(int );
    void on_fdcResistance_valueChanged(double );
    void on_saveButton_clicked();
    void on_openButton_clicked();
    void on_motPinsButton_clicked();
    void on_tabWidget_currentChanged(int index);
    void on_toolButtonAdd_clicked();
    void on_toolButtonSuppr_clicked();
    void on_buttonPrev_clicked();
    void on_buttonNext_clicked();
    void on_cnySchPullup_currentIndexChanged(int index);
    void on_comboComparateur_currentIndexChanged(int index);
    void on_doubleRd_valueChanged(double );
    void on_doubleRt_valueChanged(double );
    void on_doubleRc_valueChanged(double );
    void on_doubleVseuil_valueChanged(double );
    void on_doubleX_valueChanged(double );
    void on_doubleY_valueChanged(double );
    void on_doubleH_valueChanged(double );
    void on_cnyPin_currentIndexChanged(int index);
    void on_checkBoxTestBlanc_toggled(bool checked);
    void on_doubleMoquette_valueChanged(double );
    void on_doubleScotch_valueChanged(double );
    void on_doubleVoie_valueChanged(double );
    void on_doubleVbat_valueChanged(double );
    void on_doubleMasse_valueChanged(double );
    void on_doubleKroul_valueChanged(double );
    void on_doubleDiametreRoue_valueChanged(double );
    void on_doubleRapport_valueChanged(double );
    void on_doubleRendement_valueChanged(double );
    void on_doubleVdiode_valueChanged(double );
    void on_doubleRdson_valueChanged(double );
    void on_doubleRmotG_valueChanged(double );
    void on_doubleLmotG_valueChanged(double );
    void on_doubleKphiG_valueChanged(double );
    void on_doubleInertieG_valueChanged(double );
    void on_doubleI0motG_valueChanged(double );
    void on_toolButtonDiffMot_toggled(bool checked);
    void on_doubleRmotD_valueChanged(double );
    void on_doubleLmotD_valueChanged(double );
    void on_doubleKphiD_valueChanged(double );
    void on_doubleInertieD_valueChanged(double );
    void on_doubleI0motD_valueChanged(double );

    void fdcClick(bool );

public slots :
    void viewModele();
    void viewCaptIrModele();
    void moveCapteur();
    void fdcChange();
    void afficheDataTest();
    void jackToggle();
    void appuiFdc();
    void relacheFdc();
};

#endif // GAMEL_H
