#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QTime>
#include <QLabel>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QMdiSubWindow>
#include <QTranslator>

#include <math.h>
#include "general.h"
#include "fenetrecartemicro.h"
#include "fenetreoscillos.h"
#include "fenetrepiste.h"
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

    Apropos *apropos;

    int timerID, speed;
    bool validTimer;

    void timerEvent(QTimerEvent *event);

protected:
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

private slots:
    void on_speedSpinBox_valueChanged(double );
    void on_speedSlider_valueChanged(int value);
    void action_apropos() {  apropos->show();  }
    void on_action_Mode_simple_triggered();
    void on_action_Mode_complet_triggered();

public slots :
    void stopTimer() { if (validTimer) { killTimer(timerID); validTimer=false; } }
    void newTimer();
    void organiser();
};

#endif // MAINWINDOW_H
