#include <math.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "diagnostic.h"

#include <QMdiSubWindow>
#include <QTextCodec>
#include <QFile>
#include <QScrollBar>
#include <QTimer>
#include <QCloseEvent>
#include <QApplication>
#include <QThread>
#include <QProgressDialog>

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

    // Fichier d'initialisation : recharger au demarrage le dernier firmware
    // utilise s'il existe (charge, pas demarre ; Power lance l'ESP32).
    micro->chargeFirmwareRecent();

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

    // Fichier d'initialisation : recharger la derniere gamelle utilisee
    // si elle existe (chargee, non affichee : la fenetre Gamel est masquee).
    gamel->chargeGamelleRecent();

    piste = new FenetrePiste(modele);
    subWindow4 = new QMdiSubWindow;
    subWindow4->setWidget(piste);
    subWindow4->setWindowIcon(QIcon(":/images/icone_piste.png"));
    ui->mdiArea->addSubWindow(subWindow4, Qt::SubWindow);

    connect(ui->action_Piste, SIGNAL(triggered()), subWindow4, SLOT(show()));
    connect(ui->action_Piste, SIGNAL(triggered()), subWindow4, SLOT(setFocus()));

    // Fichier d'initialisation : recharger la derniere piste utilisee si
    // elle existe, au lieu de la piste par defaut (piste001.pis embarquee).
    piste->chargePisteRecent();

    terminal = new FenetreTerminal(modele);
    subWindow5 = new QMdiSubWindow;
    subWindow5->setWidget(terminal);
    subWindow5->setWindowIcon(QIcon(":/images/icone_terminal.png"));
    ui->mdiArea->addSubWindow(subWindow5, Qt::SubWindow);

    connect(ui->action_Terminal, SIGNAL(triggered()), subWindow5, SLOT(show()));
    connect(ui->action_Terminal, SIGNAL(triggered()), subWindow5, SLOT(setFocus()));

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
    subWindow5->hide();
    ui->action_Terminal->setDisabled(true);
    ui->action_Terminal->setVisible(false);
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
    subWindow5->show();
    ui->action_Terminal->setDisabled(false);
    ui->action_Terminal->setVisible(true);
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
    h = h1 + ui->menuBar->frameGeometry().height() + deltaH + 8;
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

// ==================== Mode diagnostic ====================
void MainWindow::demarrageDiagnostic(const QString &firmware, const QString &fichierGamelle)
{
    // Gamelle pre-chargee depuis la CLI (--gamelle) : equivalent de
    // "Charger les donnees d'une gamelle" dans fenetre Gamel (modele->openData)
    if (!fichierGamelle.isEmpty()) {
        if (QFile::exists(fichierGamelle)) {
            modele.openData(fichierGamelle);
            qDebug() << "Diagnostic: gamelle chargee" << fichierGamelle;
        } else {
            qWarning() << "Diagnostic: gamelle absente" << fichierGamelle;
        }
    }
    if (!modele.loadEsp32Firmware(firmware)) {
        qWarning() << "Diagnostic: impossible de charger le firmware" << firmware;
        return;
    }
    qDebug() << "Diagnostic: demarrage ESP32 avec" << firmware;
    // ESPSIMU_DIAG_JACK=1 : fiche jack posee au demarrage puis retiree a 3 s,
    // equivalent headless du bouton jack de la fenetre gamelle. C'est le retrait
    // (contact revenu au repos : IO23 tiree haute par le pull-up) qui demarre
    // la gamelle dans le firmware suivi ; la pose seule la laisse en attente.
    if (qEnvironmentVariable("ESPSIMU_DIAG_JACK") == "1") {
        modele.metJack(true);
        qDebug() << "Diagnostic: jack pose";
        QTimer::singleShot(3000, this, [this]() {
            modele.metJack(false);
            qDebug() << "Diagnostic: jack retire (depart gamelle)";
        });
    }
    modele.startEsp32();

    // Reset toutes les 8 s : le premier tombe apres le premier tour(s) de loop
    // (rampe PWM ~1.7 s par tour), les suivants re-exposent le phenomene. 5 cycles.
    // ESPSIMU_DIAG_NORESET=1 : pas de reset auto (session longue sans reset,
    // pour reproduire un usage normal tel que l'utilisateur le voit).
    if (qEnvironmentVariable("ESPSIMU_DIAG_NORESET") != "1") {
        QTimer *horlogeReset = new QTimer(this);
        connect(horlogeReset, &QTimer::timeout, this, [this, horlogeReset, n = 0]() mutable {
            qDebug() << "Diagnostic: reset automatique";
            modele.resetEsp32();
            if (++n >= 5) horlogeReset->stop();
        });
        horlogeReset->start(8000);
    }

    // ESPSIMU_DIAG_OFFON=1 : cycles extinction/relancement (power off/on),
    // equivalent headless du bouton Power de la fenetre carte. Deja deux
    // cycles a t=6/9 s puis 12/15 s (apres le boot initial) : reproduit le
    // banc "mise hors tension puis sous tension".
    if (qEnvironmentVariable("ESPSIMU_DIAG_OFFON") == "1") {
        QTimer::singleShot(6000, this, [this]() {
            qDebug() << "Diagnostic: extinction (power off)";
            modele.stopEsp32();
        });
        QTimer::singleShot(9000, this, [this]() {
            qDebug() << "Diagnostic: relance (power on)";
            modele.startEsp32();
        });
        QTimer::singleShot(12000, this, [this]() {
            qDebug() << "Diagnostic: extinction (power off)";
            modele.stopEsp32();
        });
        QTimer::singleShot(15000, this, [this]() {
            qDebug() << "Diagnostic: relance (power on)";
            modele.startEsp32();
        });
    }

    // Fermeture automatique pour les essais en batch (ESPSIMU_DIAG_AUTOCLOSE=1
    // → 50 s par defaut, ou une valeur en millisecondes, ex. ESPSIMU_DIAG_AUTOCLOSE=180000)
    QString autoclose = qEnvironmentVariable("ESPSIMU_DIAG_AUTOCLOSE");
    if (!autoclose.isEmpty() && autoclose != "0") {
        int ms;
        if (autoclose == "1") {
            // forme historique : ESPSIMU_DIAG_AUTOCLOSE=1 -> 50 s
            ms = 50000;
        } else {
            bool ok = false;
            ms = autoclose.toInt(&ok);
            if (!ok || ms <= 0) ms = 50000;
        }
        QTimer::singleShot(ms, this, [this]() { close(); });
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Deuxieme passage : on ferme pour de vrai
    if (m_closing) {
        EspDiag::mark("close pass2 : accept + quit");
        event->accept();
        // Quitter explicitement : la QProgressDialog d'arret, jamais fermee
        // (WA_DeleteOnClose), reste alors la seule fenetre visible et
        // quitOnLastWindowClosed ne declenche jamais — l'app restait vivante
        // indefiniment (event loop en ppoll, thread QEMU attache, TERM ignore
        // par le handler de signal installe par la lib QEMU).
        QApplication::quit();
        return;
    }

    // Premier passage : on refuse et on affiche la popup
    event->ignore();
    m_closing = true;

    qDebug() << "Fermeture : arret de l'ESP32...";
    EspDiag::mark("close pass1 : ignore + popup");

    QProgressDialog *progress = new QProgressDialog("Fermeture en cours...",
                                                    nullptr, 0, 0, this);
    progress->setWindowTitle("EspSimu");
    progress->setWindowModality(Qt::ApplicationModal);
    progress->setMinimumDuration(0);
    progress->setCancelButton(nullptr);
    progress->setAttribute(Qt::WA_DeleteOnClose);
    progress->show();

    // Laisser la popup s'afficher, PUIS arreter l'ESP32
    QTimer::singleShot(200, this, [this]() {
        // Fermeture de la fenetre = fin de l'application : arret complet du
        // thread QEMU (shutdownEsp32 demande l'arret de la main-loop et
        // l'attend), pas une simple pause — cette derniere reste reservee au
        // bouton Power.
        EspDiag::mark("close timer : appelle shutdownEsp32");
        modele.shutdownEsp32();
        EspDiag::mark("close timer : shutdownEsp32 rendu, close()");
        close();   // relance closeEvent, mais m_closing=true -> accepte + quit()
    });
}
