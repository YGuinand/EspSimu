#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QLabel>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QMdiSubWindow>
#include <QTranslator>

#include <math.h>
#include "fenetrecartemicro.h"
#include "fenetreoscillos.h"
#include "fenetrepiste.h"
#include "fenetreterminal.h"
#include "modele.h"
#include "gamel.h"
#include "apropos.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    // Mode diagnostic en ligne de commande (--diag-autostart <firmware>) :
    // charge et demarre un firmware d'un seul tenant (utilisable hors ecran :
    // QT_QPA_PLATFORM=offscreen), puis enchaine des reset toutes les 8 s
    // (apres le premier tour de loop) pour reproduire et mesurer le lag
    // post-reset. Fermeture auto si ESPSIMU_DIAG_AUTOCLOSE=1.
    // --gamelle <fichier.gml> pre-charge la gamelle (cablage/capteurs/hacheurs)
    // avant le firmware, pour reproduire le scénario complet sans passer par
    // les dialogues.
    void demarrageDiagnostic(const QString &firmware, const QString &fichierGamelle = QString());

private:
    Ui::MainWindow *ui;

    Modele modele;

    QMdiSubWindow *subWindow1;
    FenetreCarteMicro *micro;

    QMdiSubWindow *subWindow2;
    FenetreOscillos *oscillos;

    QMdiSubWindow *subWindow3;
    Gamel *gamel;

    QMdiSubWindow *subWindow4;
    FenetrePiste *piste;

    QMdiSubWindow *subWindow5;
    FenetreTerminal *terminal;

    Apropos *apropos;

    bool m_closing = false;

protected:
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);
    void closeEvent(QCloseEvent *event);

private slots:
    void action_apropos() {  apropos->show();  }
    void on_action_Mode_simple_triggered();
    void on_action_Mode_complet_triggered();

public slots :
    void organiser();
};

#endif // MAINWINDOW_H
