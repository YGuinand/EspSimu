#include <math.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMdiSubWindow>
#include <QTextCodec>
#include <QScrollBar>

// #define VERSION_SANS_GAMELLE

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

//    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
//    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));

    connect(ui->action_Organiser, SIGNAL(triggered()), SLOT(organiser()));

    micro = new FenetreCarteMicro(modele);
    subWindow1 = new QMdiSubWindow;
    subWindow1->setWidget(micro);
    subWindow1->setWindowIcon(QIcon(":/images/icone_lcd.png"));
    ui->mdiArea->addSubWindow(subWindow1, Qt::SubWindow);
//    ui->mdiArea->addSubWindow(subWindow1, Qt::Tool);
//    subWindow1->setFixedSize(micro->size());

    connect(ui->action_Microcontroleur, SIGNAL(triggered()), subWindow1, SLOT(show()));
    connect(ui->action_Microcontroleur, SIGNAL(triggered()), subWindow1, SLOT(setFocus()));

    connect(ui->action_Ouvrir_un_programme, SIGNAL(triggered()), micro, SLOT(on_pushButtonOuvrir_clicked()));
    connect(ui->action_Recharger_le_programme, SIGNAL(triggered()), micro, SLOT(on_pushButtonRefresh_clicked()));
    connect(micro, SIGNAL(refreshEnabled(bool)), ui->action_Recharger_le_programme, SLOT(setEnabled(bool)));

    oscillos = new FenetreOscillos(modele);
    subWindow2 = new QMdiSubWindow;
    subWindow2->setWidget(oscillos);
    subWindow2->setWindowIcon(QIcon(":/images/icone_oscillo.png"));
    ui->mdiArea->addSubWindow(subWindow2, Qt::SubWindow);
//    ui->mdiArea->addSubWindow(subWindow2, Qt::Tool);

    connect(ui->action_Oscilloscopes, SIGNAL(triggered()), subWindow2, SLOT(show()));
    connect(ui->action_Oscilloscopes, SIGNAL(triggered()), subWindow2, SLOT(setFocus()));

    gamel = new Gamel(modele);
    subWindow3 = new QMdiSubWindow;
    subWindow3->setWidget(gamel);
    subWindow3->setWindowIcon(QIcon(":/images/icone_gamel.png"));
//    subWindow3->setFixedSize(gamel->size());
    ui->mdiArea->addSubWindow(subWindow3, Qt::SubWindow);
//    ui->mdiArea->addSubWindow(subWindow3, Qt::Tool);

    connect(ui->action_Gamelle, SIGNAL(triggered()), subWindow3, SLOT(show()));
    connect(ui->action_Gamelle, SIGNAL(triggered()), subWindow3, SLOT(setFocus()));

    piste = new FenetrePiste(modele);
    subWindow4 = new QMdiSubWindow;
    subWindow4->setWidget(piste);
    subWindow4->setWindowIcon(QIcon(":/images/icone_piste.png"));
    ui->mdiArea->addSubWindow(subWindow4, Qt::SubWindow);

    connect(ui->action_Piste, SIGNAL(triggered()), subWindow4, SLOT(show()));
    connect(ui->action_Piste, SIGNAL(triggered()), subWindow4, SLOT(setFocus()));

    speed = 1;
    validTimer = false;
    ui->speedSpinBox->setValue(speed);
    connect(&modele, SIGNAL(setPowerOn()), this, SLOT(newTimer()));
    connect(&modele, SIGNAL(setPowerOff()), this, SLOT(stopTimer()));

    connect(ui->action_Barre_de_vitesse, SIGNAL(toggled(bool)), ui->widgetSpeedBar, SLOT(setVisible(bool)));

    apropos = new Apropos();
    connect(ui->action_propos, SIGNAL(triggered()), SLOT(action_apropos()));

    subWindow3->hide();
    subWindow4->hide();
#ifdef VERSION_SANS_GAMELLE
    on_action_Mode_simple_triggered();
    ui->action_Mode_complet->setVisible(false);
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    for (int i=0; i<event->count(); i++) {
        if (event->text().at(i)=='0') micro->set_bp0Pressed();
        else if (event->text().at(i)=='1') micro->set_bp1Pressed();
        else if (event->text().at(i)=='2') micro->set_bp2Pressed();
        else if (event->text().at(i)=='+') micro->augmenteAn0();
        else if (event->text().at(i)=='-') micro->diminueAn0();
        else if ((event->text().at(i)=='r')||(event->text().at(i)=='R')) micro->set_resetPressed();
        else if ((event->text().at(i)=='j')||(event->text().at(i)=='J')) gamel->jackToggle();
        else if ((event->text().at(i)=='f')||(event->text().at(i)=='F')) gamel->appuiFdc();
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    for (int i=0; i<event->count(); i++) {
        if (event->text().at(i)=='0') micro->set_bp0Released();
        else if (event->text().at(i)=='1') micro->set_bp1Released();
        else if (event->text().at(i)=='2') micro->set_bp2Released();
        else if ((event->text().at(i)=='r')||(event->text().at(i)=='R')) micro->set_resetReleased();
        else if ((event->text().at(i)=='f')||(event->text().at(i)=='F')) gamel->relacheFdc();
    }
}

void MainWindow::on_action_Mode_simple_triggered()
{
    ui->action_Mode_simple->setVisible(false);
    ui->action_Mode_complet->setVisible(true);
    subWindow3->hide();
    subWindow4->hide();
    subWindow2->hide();
    ui->action_Gamelle->setDisabled(true);
    ui->action_Gamelle->setVisible(false);
    ui->action_Piste->setDisabled(true);
    ui->action_Piste->setVisible(false);
    ui->action_Organiser->setVisible(true);
    ui->action_Organiser->setDisabled(false);
    ui->action_Cascader->setVisible(false);
    show();
    resize(subWindow1->frameSize().width(),
           subWindow1->frameGeometry().height()+
           ui->widgetSpeedBar->frameGeometry().height()+
           ui->menuBar->frameGeometry().height());
    organiser();
    subWindow1->showMaximized();
    connect(ui->action_Microcontroleur, SIGNAL(triggered()), SLOT(organiser()));
    connect(ui->action_Oscilloscopes, SIGNAL(triggered()), SLOT(organiser()));
    connect(micro, SIGNAL(estCache(bool)), SLOT(organiser()));
    connect(oscillos, SIGNAL(estCache(bool)), SLOT(organiser()));
}


void MainWindow::on_action_Mode_complet_triggered()
{
    ui->action_Mode_simple->setVisible(true);
    ui->action_Mode_complet->setVisible(false);
    subWindow1->show();
    subWindow3->show();
    subWindow4->show();
    subWindow2->show();
    ui->action_Gamelle->setDisabled(false);
    ui->action_Gamelle->setVisible(true);
    ui->action_Piste->setDisabled(false);
    ui->action_Piste->setVisible(true);
    ui->action_Organiser->setVisible(false);
    ui->action_Organiser->setDisabled(true);
    ui->action_Cascader->setVisible(true);
    show();
    organiser();
    subWindow1->showNormal();
    disconnect(ui->action_Microcontroleur, SIGNAL(triggered()), this, SLOT(organiser()));
    disconnect(ui->action_Oscilloscopes, SIGNAL(triggered()), this, SLOT(organiser()));
    disconnect(micro, SIGNAL(estCache(bool)), this, SLOT(organiser()));
    disconnect(oscillos, SIGNAL(estCache(bool)), this, SLOT(organiser()));
}

void MainWindow::timerEvent(QTimerEvent * /* event */)
{
    static QElapsedTimer mesure;
    static bool isStarted = false;
    static int calcul = 0;

    if (!isStarted) {
        mesure.start();
        isStarted = true;
    }
    calcul -= mesure.elapsed();

    static int boucle = 0;

    modele.clock();

    ui->labelTime->setText(QLocale().toString(modele.getProcesseur().time()/48e6,'f',3));
    boucle++;

    if ((boucle % 10) == 0) oscillos->miseAJour();

    calcul += mesure.elapsed();
    if (boucle>=100) {
        int pc = 70 * (calcul+100) / (speed*100);
        if (pc>=70) {
            ui->progressBar->setStyleSheet("::chunk { background-color: rgb(250,0,0); }");
            if (pc>100) pc=100;
        } else if (pc>=50) {
            ui->progressBar->setStyleSheet("::chunk { background-color: rgb(255,170,40); }");
        } else {
            ui->progressBar->setStyleSheet("::chunk { background-color: rgb(0,220,0); }");
        }
        ui->progressBar->setValue(pc);
        int duree = mesure.restart();
        ui->speedLabel->setText(QLocale().toString(duree/1000.0,'f',3));  // setNum(duree/1000.0);
        boucle = 0;
        calcul = 0;
    }
}

void MainWindow::newTimer()
{
    if (validTimer) killTimer(timerID);
    timerID = startTimer(speed);
    validTimer = true;
}

void MainWindow::on_speedSlider_valueChanged(int value)
{
    speed = value;
    ui->speedSpinBox->setValue(value/10.0);
    if (validTimer) newTimer();
}

void MainWindow::on_speedSpinBox_valueChanged(double value)
{
    ui->speedSlider->setValue(value*10);
}

void MainWindow::organiser()
{
    int deltaH = 0, deltaW = 0, h, w, h1, w1;
    subWindow1->showNormal();
    subWindow1->move(0,0);
    w1 = subWindow1->frameGeometry().width();
    h1 = subWindow1->frameGeometry().height();
    if (ui->mdiArea->horizontalScrollBar()) {
        if (ui->mdiArea->horizontalScrollBar()->isVisible()) deltaH = ui->mdiArea->horizontalScrollBar()->height();
    }
    if (ui->mdiArea->verticalScrollBar()) {
        if (ui->mdiArea->verticalScrollBar()->isVisible()) deltaW = ui->mdiArea->verticalScrollBar()->width();
    }
    h = h1 + ui->widgetSpeedBar->frameGeometry().height() +
             ui->menuBar->frameGeometry().height() + deltaH + 8;
    if (subWindow2->isVisible()) {
        w = w1*2 + deltaW + 1;
        subWindow2->showNormal();
        subWindow2->resize(w1, h1);
        subWindow2->move(w1-1,0);
    } else {
        w = subWindow1->frameSize().width() + deltaW + 2;
    }
    resize(w, h);
}
