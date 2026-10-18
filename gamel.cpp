#include "gamel.h"
#include "ui_gamel.h"

#include <QFileDialog>
#include <QGraphicsProxyWidget>

Gamel::Gamel(Modele &m, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::Gamel),
    scene(-150, -10, 300, 320),
    echelle(300.0/240.0), delta(6.5)
{
    ui->setupUi(this);

    fdc = new QCheckBox();
    fdc->setStyleSheet("QCheckBox { background: transparent; }\n"
       "QCheckBox::indicator:unchecked { image: url(:/images/fdc_off.png); }\n"
       "QCheckBox::indicator:checked { image: url(:/images/fdc_on.png); }\n"
       "QCheckBox::indicator:unchecked:pressed { image: url(:/images/fdc_on.png); }\n"
       "QCheckBox::indicator:checked:pressed { image: url(:/images/fdc_off.png); }\n"
    );

    QGraphicsProxyWidget *proxy = scene.addWidget(fdc);
    proxy->setPos(-42,-8);
    for (int i=0; i<Modele::NB_MAX_CAPT_IR; i++) {
        tabItems[i] = scene.addPixmap(QPixmap(":/images/cny70bleu.png"));
        tabItems[i]->hide();
    }
    focusItem = new FocusItemCNY(
                    changeRepX(ui->doubleX->minimum()),
                    changeRepY(ui->doubleY->maximum()),
                    changeRepX(ui->doubleX->maximum()),
                    changeRepY(ui->doubleY->minimum()));
    scene.addItem(focusItem);
    focusItem->hide();

    ui->label_axes->hide();
    ui->widgetExtension->hide();
    ui->tabWidget->setCurrentIndex(0);

    ui->graphicsView->setScene(&scene);

    fichier = "gamel001";
    pwm1aGauche = true;
    captIrIndex = 0;

    modele = &m;

    viewModele();

    connect(ui->jack, SIGNAL(toggled(bool)), modele, SLOT(metJack(bool)));

//    connect(fdc, SIGNAL(toggled(bool)), modele, SLOT(appuyeFdc(bool)));

    connect(fdc, SIGNAL(toggled(bool)), this, SLOT(fdcClick(bool)));
    connect(fdc, SIGNAL(pressed()), modele, SLOT(appuyeFdc()));
    connect(fdc, SIGNAL(released()), modele, SLOT(relacheFdc()));

    connect(modele, SIGNAL(openNew()), this, SLOT(viewModele()));
    connect(focusItem, SIGNAL(moved()), this, SLOT(moveCapteur()));
    connect(modele, SIGNAL(fdcChange()), this, SLOT(fdcChange()));
    connect(modele, SIGNAL(captCNYChange()), this, SLOT(afficheDataTest()));
}

void Gamel::changePinsInUse()
{
    remplirComboJ2(*ui->jackPin, modele->getJackPinIndex());
    remplirComboJ2(*ui->fdcPin, modele->getFdcPinIndex());
    if (modele->nbCaptIr()>0) remplirComboJ2(*ui->cnyPin, modele->getCaptIr(captIrIndex)->getJ2Pin());
}

void Gamel::viewModele()
{
    // fiche jack
    ui->jackSchema->blockSignals(true);
    if (modele->getJack().getPullUp()) ui->jackSchema->setCurrentIndex(0); else ui->jackSchema->setCurrentIndex(1);
    ui->jackSchema->blockSignals(false);
    ui->jackContact->blockSignals(true);
    if (modele->getJack().getContactNO()) ui->jackContact->setCurrentIndex(0); else ui->jackContact->setCurrentIndex(1);
    ui->jackContact->blockSignals(false);
    ui->jackResistance->blockSignals(true);
    ui->jackResistance->setValue(modele->getJack().getResistance()/1000.0);
    ui->jackResistance->blockSignals(false);
    ui->jack->blockSignals(true);
    ui->jack->setChecked(modele->getJack().isActif());
    ui->jack->blockSignals(false);
    remplirComboJ2(*ui->jackPin, modele->getJackPinIndex());
    ui->schemaPinJack->setText(ui->jackPin->currentText());
    changerJackSchema();

    // contact fdc
    ui->fdcSchema->blockSignals(true);
    if (modele->getFdc().getPullUp()) ui->fdcSchema->setCurrentIndex(0); else ui->fdcSchema->setCurrentIndex(1);
    ui->fdcSchema->blockSignals(false);
    ui->fdcContact->blockSignals(true);
    if (modele->getFdc().getContactNO()) ui->fdcContact->setCurrentIndex(0); else ui->fdcContact->setCurrentIndex(1);
    ui->fdcContact->blockSignals(false);
    ui->fdcResistance->blockSignals(true);
    ui->fdcResistance->setValue(modele->getFdc().getResistance()/1000.0);
    ui->fdcResistance->blockSignals(false);
    fdc->blockSignals(true);
    fdc->setChecked(modele->getFdc().isActif());
    fdc->blockSignals(false);
    remplirComboJ2(*ui->fdcPin, modele->getFdcPinIndex());
    ui->schemaPinFdc->setText(ui->fdcPin->currentText());
    changerFdcSchema();

    // pwm
    if (modele->isPwm1aGauche() != pwm1aGauche) on_motPinsButton_clicked();

    // capteurs IR
    viewCaptIrModele();

    // parametres generaux
    ui->doubleScotch->blockSignals(true);
    ui->doubleScotch->setValue(CaptIrCNX::getRefScotch());
    ui->doubleScotch->blockSignals(false);
    ui->doubleMoquette->blockSignals(true);
    ui->doubleMoquette->setValue(CaptIrCNX::getRefMoquette());
    ui->doubleMoquette->blockSignals(false);
    ui->doubleVoie->blockSignals(true);
    ui->doubleVoie->setValue(modele->getVoie());
    ui->doubleVoie->blockSignals(false);
    ui->doubleVbat->blockSignals(true);
    ui->doubleVbat->setValue(modele->getVbat());
    ui->doubleVbat->blockSignals(false);
    ui->doubleMasse->blockSignals(true);
    ui->doubleMasse->setValue(modele->getMasse());
    ui->doubleMasse->blockSignals(false);
    ui->doubleKroul->blockSignals(true);
    ui->doubleKroul->setValue(modele->getKroul());
    ui->doubleKroul->blockSignals(false);
    ui->doubleRapport->blockSignals(true);
    ui->doubleRapport->setValue(modele->getReduc());
    ui->doubleRapport->blockSignals(false);
    ui->doubleDiametreRoue->blockSignals(true);
    ui->doubleDiametreRoue->setValue(modele->getRayon()*2.0);
    ui->doubleDiametreRoue->blockSignals(false);
    ui->doubleRendement->blockSignals(true);
    ui->doubleRendement->setValue(modele->getRendement()*100.0);
    ui->doubleRendement->blockSignals(false);
    ui->doubleVdiode->blockSignals(true);
    ui->doubleVdiode->setValue(modele->getVf());
    ui->doubleVdiode->blockSignals(false);
    ui->doubleRdson->blockSignals(true);
    ui->doubleRdson->setValue(modele->getRdson());
    ui->doubleRdson->blockSignals(false);

    // moteur gauche
    ui->doubleRmotG->blockSignals(true);
    ui->doubleRmotG->setValue(modele->getMoteurG()->getRes());
    ui->doubleRmotG->blockSignals(false);
    ui->doubleLmotG->blockSignals(true);
    ui->doubleLmotG->setValue(modele->getMoteurG()->getInduc() * 1.0e3);
    ui->doubleLmotG->blockSignals(false);
    ui->doubleKphiG->blockSignals(true);
    ui->doubleKphiG->setValue(modele->getMoteurG()->getKphi());
    ui->doubleKphiG->blockSignals(false);
    ui->doubleInertieG->blockSignals(true);
    ui->doubleInertieG->setValue(modele->getMoteurG()->getInertie());
    ui->doubleInertieG->blockSignals(false);
    ui->doubleI0motG->blockSignals(true);
    ui->doubleI0motG->setValue(modele->getMoteurG()->getI0());
    ui->doubleI0motG->blockSignals(false);

    // moteur droit
    ui->doubleRmotD->blockSignals(true);
    ui->doubleRmotD->setValue(modele->getMoteurD()->getRes());
    ui->doubleRmotD->blockSignals(false);
    ui->doubleLmotD->blockSignals(true);
    ui->doubleLmotD->setValue(modele->getMoteurD()->getInduc() * 1.0e3);
    ui->doubleLmotD->blockSignals(false);
    ui->doubleKphiD->blockSignals(true);
    ui->doubleKphiD->setValue(modele->getMoteurD()->getKphi());
    ui->doubleKphiD->blockSignals(false);
    ui->doubleInertieD->blockSignals(true);
    ui->doubleInertieD->setValue(modele->getMoteurD()->getInertie());
    ui->doubleInertieD->blockSignals(false);
    ui->doubleI0motD->blockSignals(true);
    ui->doubleI0motD->setValue(modele->getMoteurD()->getI0());
    ui->doubleI0motD->blockSignals(false);
}

void Gamel::viewCaptIrModele()
{
    // capt ir
    if (captIrIndex<0) captIrIndex=0;
    if (captIrIndex>=modele->nbCaptIr()) captIrIndex=modele->nbCaptIr()-1;
    for (int i=0; i<modele->nbCaptIr(); i++) {
        CaptIrCNX *c = modele->getCaptIr(i);
        tabItems[i]->setPos(changeRepX(c->getX()), changeRepY(c->getY()));
        tabItems[i]->show();
    }
    for (int i=modele->nbCaptIr(); i<Modele::NB_MAX_CAPT_IR; i++) tabItems[i]->hide();
    if (captIrIndex<0) { // pas de capteur IR
        QList<QWidget*> list = ui->tabCapteurs->findChildren<QWidget*>();
        foreach(QWidget* w, list) w->setEnabled(false);
        list = ui->widgetExtension->findChildren<QWidget*>();
        foreach(QWidget* w, list) w->setEnabled(false);
        ui->toolButtonAdd->setEnabled(true);
        ui->labelNumero->setText("0/0");
        focusItem->hide();
    } else {
        QList<QWidget*> list = ui->tabCapteurs->findChildren<QWidget*>();
        foreach(QWidget* w, list) w->setEnabled(true);
        list = ui->widgetExtension->findChildren<QWidget*>();
        foreach(QWidget* w, list) w->setEnabled(true);
        int nb = modele->nbCaptIr();
        ui->labelNumero->setText(QString().setNum(captIrIndex+1)+"/"+QString().setNum(nb));
        ui->toolButtonAdd->setEnabled(modele->captIrAjoutPossible());
        ui->buttonPrev->setDisabled(captIrIndex==0);
        ui->buttonNext->setDisabled((captIrIndex+1)==nb);
        CaptIrCNX &c = *modele->getCaptIr(captIrIndex);
        ui->cnySchPullup->blockSignals(true);
        ui->cnySchPullup->setCurrentIndex((int)(!c.getPullUp()));
        ui->cnySchPullup->blockSignals(false);
        int index;
        if (c.getNoComp()) index = 0;
        else if (c.getSup()) index = 1; else index = 2;
        ui->comboComparateur->blockSignals(true);
        ui->comboComparateur->setCurrentIndex(index);
        ui->comboComparateur->blockSignals(false);
        ui->doubleRc->setDisabled(index==0);
        ui->doubleVseuil->setDisabled(index==0);
        ui->doubleRd->blockSignals(true);
        ui->doubleRd->setValue(c.getRd()/1000.0);
        ui->doubleRd->blockSignals(false);
        ui->doubleRt->blockSignals(true);
        ui->doubleRt->setValue(c.getRt()/1000.0);
        ui->doubleRt->blockSignals(false);
        ui->doubleRc->blockSignals(true);
        ui->doubleRc->setValue(c.getRc()/1000.0);
        ui->doubleRc->blockSignals(false);
        ui->doubleVseuil->blockSignals(true);
        ui->doubleVseuil->setValue(c.getVseuil());
        ui->doubleVseuil->blockSignals(false);
        ui->doubleX->blockSignals(true);
        ui->doubleX->setValue(c.getX());
        ui->doubleX->blockSignals(false);
        ui->doubleY->blockSignals(true);
        ui->doubleY->setValue(c.getY());
        ui->doubleY->blockSignals(false);
        ui->doubleH->blockSignals(true);
        ui->doubleH->setValue(c.getH());
        ui->doubleH->blockSignals(false);
        remplirComboJ2(*ui->cnyPin, modele->getCaptIr(captIrIndex)->getJ2Pin());
        ui->schemaPinCny->setText(ui->cnyPin->currentText());
        changerCaptIrSchema();
        focusItem->blockSignals(true);
        focusItem->setPos(tabItems[captIrIndex]->pos());
        focusItem->setVisible(ui->tabWidget->currentIndex()==ui->tabWidget->indexOf(ui->tabCapteurs));
        focusItem->blockSignals(false);
        ui->checkBoxTestBlanc->blockSignals(true);
        ui->checkBoxTestBlanc->setChecked(c.estSurLigne());
        ui->checkBoxTestBlanc->blockSignals(false);
        afficheDataTest();
    }
}

Gamel::~Gamel()
{
    delete ui;
}

// Liste des entrees du connecteur J2 : la broche courante en premier, puis NC, puis les broches libres
void Gamel::remplirComboJ2(QComboBox &c, int indexCourant)
{
    std::vector<QString> tab;
    std::vector<int> tabIndex;
    modele->getUnusedJ2(tab, tabIndex);

    c.blockSignals(true);
    c.clear();
    if (indexCourant>=0) {
        c.addItem(Modele::j2Name(indexCourant), QVariant(indexCourant));
    }
    c.addItem(Connexion::NCString, QVariant(-1));
    for (unsigned int j=0; j<tab.size(); j++) {
        c.addItem(tab[j], QVariant(tabIndex[j]));
    }
    c.setCurrentIndex(0);
    c.blockSignals(false);
}

void Gamel::changerJackSchema()
{
    ContactCNX &c = modele->getJack();
    if (c.getContactNO()) { // NO
        if (c.getPullUp()) { // pull-up
            ui->schemaJack->setPixmap(QPixmap(":/images/no_up.png"));
        } else {  // pull-down
            ui->schemaJack->setPixmap(QPixmap(":/images/no_down.png"));
        }
    } else { // NC
        if (c.getPullUp()) { // pull-up
            ui->schemaJack->setPixmap(QPixmap(":/images/nc_up.png"));
        } else {  // pull-down
            ui->schemaJack->setPixmap(QPixmap(":/images/nc_down.png"));
        }
    }
}

void Gamel::changerFdcSchema()
{
    ContactCNX &c = modele->getFdc();
    if (c.getContactNO()) { // NO
        if (c.getPullUp()) { // pull-up
            ui->schemaFdc->setPixmap(QPixmap(":/images/no_up.png"));
        } else {  // pull-down
            ui->schemaFdc->setPixmap(QPixmap(":/images/no_down.png"));
        }
    } else { // NC
        if (c.getPullUp()) { // pull-up
            ui->schemaFdc->setPixmap(QPixmap(":/images/nc_up.png"));
        } else {  // pull-down
            ui->schemaFdc->setPixmap(QPixmap(":/images/nc_down.png"));
        }
    }
}

void Gamel::changerCaptIrSchema()
{
    CaptIrCNX &c = *modele->getCaptIr(captIrIndex);
    if (c.getPullUp()) {
        ui->sch_cny->setPixmap(QPixmap(":/images/sch_cny_up.png"));
        if (c.getNoComp()) {
            ui->sch_comp->setPixmap(QPixmap(":/images/sch_nocomp_up.png"));
            ui->spacerPinNameH->changeSize(20,15,QSizePolicy::Fixed,QSizePolicy::Fixed);
        } else {
            if (c.getSup()) {
                ui->sch_comp->setPixmap(QPixmap(":/images/sch_comp_pos_up.png"));
            } else {
                ui->sch_comp->setPixmap(QPixmap(":/images/sch_comp_neg_up.png"));
            }
            ui->spacerPinNameH->changeSize(20,5,QSizePolicy::Fixed,QSizePolicy::Fixed);
        }
    } else {
        ui->sch_cny->setPixmap(QPixmap(":/images/sch_cny_down.png"));
        if (c.getNoComp()) {
            ui->sch_comp->setPixmap(QPixmap(":/images/sch_nocomp_down.png"));
            ui->spacerPinNameH->changeSize(20,10,QSizePolicy::Fixed,QSizePolicy::Fixed);
        } else {
            if (c.getSup()) {
                ui->sch_comp->setPixmap(QPixmap(":/images/sch_comp_pos_down.png"));
            } else {
                ui->sch_comp->setPixmap(QPixmap(":/images/sch_comp_neg_down.png"));
            }
            ui->spacerPinNameH->changeSize(20,0,QSizePolicy::Fixed,QSizePolicy::Fixed);
        }
    }
    ui->layoutPinName->invalidate();
}

void Gamel::on_jackContact_currentIndexChanged(int index)
{
    modele->getJack().setContactNO(index==0);
    changerJackSchema();
}

void Gamel::on_jackSchema_currentIndexChanged(int index)
{
    modele->getJack().setPullUp(index==0);
    changerJackSchema();
}

void Gamel::on_jackPin_currentIndexChanged(int index)
{
    int pinIndex = ui->jackPin->itemData(index).toInt();
    modele->jackConnecte(pinIndex);
    changePinsInUse();
    ui->schemaPinJack->setText(ui->jackPin->currentText());
}

void Gamel::on_jackResistance_valueChanged(double value)
{
    modele->getJack().setResistance(value * 1000.0);
}

void Gamel::on_fdcPin_currentIndexChanged(int index)
{
    int pinIndex = ui->fdcPin->itemData(index).toInt();
    modele->fdcConnecte(pinIndex);
    changePinsInUse();
    ui->schemaPinFdc->setText(ui->fdcPin->currentText());
}

void Gamel::on_fdcContact_currentIndexChanged(int index)
{
    modele->getFdc().setContactNO(index==0);
    changerFdcSchema();
}

void Gamel::on_fdcSchema_currentIndexChanged(int index)
{
    modele->getFdc().setPullUp(index==0);
    changerFdcSchema();
}

void Gamel::on_fdcResistance_valueChanged(double value)
{
    modele->getFdc().setResistance(value*1000.0);
}

void Gamel::on_saveButton_clicked()
{
#ifdef Q_OS_WASM
    QString sandboxFile = "tmpSave.gml";
    modele->saveGamelData(sandboxFile);
    QFile file(sandboxFile);
    file.open(QIODeviceBase::ReadOnly);
    QByteArray data = file.readAll();
    file.close();
    QFileDialog::saveFileContent(data, "gamelDatas.gml");
#else
    QString f = QFileDialog::getSaveFileName(0, QString(tr("Sauvegarder les données d'une gamelle")), fichier, tr("Fichier GML (*.gml)"));
    if (!f.isEmpty()) {
        fichier = f;
        modele->saveGamelData(fichier);
    }
#endif
}

void Gamel::on_openButton_clicked()
{
#ifdef Q_OS_WASM
    auto fileContentReady = [this](const QString &fichier, const QByteArray &fileContent) {
        if (fichier.isEmpty()) {
            // No file was selected
        } else {
            // Use fichier and fileContent
            QString sandboxFile = "tmp.gml";
            QFile file(sandboxFile);
            file.open(QIODeviceBase::WriteOnly);
            file.write(fileContent);
            file.close();
            modele->openData(sandboxFile);
        }
    };
    QFileDialog::getOpenFileContent(tr("Fichier GML (*.gml)"),  fileContentReady);
#else
    QString f = QFileDialog::getOpenFileName(0, QString(tr("Charger les données d'une gamelle")), fichier, tr("Fichier GML (*.gml)"));
    if (!f.isEmpty()) {
        fichier = f;
        modele->openData(fichier);
    }
#endif
}

void Gamel::on_motPinsButton_clicked()
{
    QString str = ui->motDPin->text();
    ui->motDPin->setText(ui->motGPin->text());
    ui->motGPin->setText(str);
    pwm1aGauche = !pwm1aGauche;
    modele->setPwm1aGauche(pwm1aGauche);
}

void Gamel::on_tabWidget_currentChanged(int index)
{
    int indexCapt = ui->tabWidget->indexOf(ui->tabCapteurs);
    ui->widgetExtension->setVisible(index==indexCapt);
    if (ui->tabWidget->currentIndex()==indexCapt) viewCaptIrModele(); else focusItem->hide();
}

void Gamel::on_toolButtonAdd_clicked()
{
    modele->addCaptIr(captIrIndex);
    captIrIndex = modele->nbCaptIr()-1;
    viewCaptIrModele();
}

void Gamel::on_toolButtonSuppr_clicked()
{
    modele->deleteCaptIr(captIrIndex);
    if (captIrIndex>0) captIrIndex--;
    viewCaptIrModele();
    changePinsInUse();
}

void Gamel::on_buttonPrev_clicked()
{
    if (captIrIndex>0) {
        captIrIndex--;
        viewCaptIrModele();
    }
}

void Gamel::on_buttonNext_clicked()
{
    if (captIrIndex+1<modele->nbCaptIr()) {
        captIrIndex++;
        viewCaptIrModele();
    }
}

void Gamel::on_cnySchPullup_currentIndexChanged(int index)
{
    modele->getCaptIr(captIrIndex)->setPullUp(index==0);
    changerCaptIrSchema();
    afficheDataTest();
}

void Gamel::on_comboComparateur_currentIndexChanged(int index)
{
    modele->getCaptIr(captIrIndex)->setNoComp(index==0);
    modele->getCaptIr(captIrIndex)->setSup(index!=2);
    viewCaptIrModele();
}

void Gamel::on_doubleRd_valueChanged(double r)
{
    modele->getCaptIr(captIrIndex)->setRd(r*1000);
    afficheDataTest();
}

void Gamel::on_doubleRt_valueChanged(double r)
{
    modele->getCaptIr(captIrIndex)->setRt(r*1000);
    afficheDataTest();
}

void Gamel::on_doubleRc_valueChanged(double r)
{
    modele->getCaptIr(captIrIndex)->setRc(r*1000);
    afficheDataTest();
}

void Gamel::on_doubleVseuil_valueChanged(double v)
{
    modele->getCaptIr(captIrIndex)->setVseuil(v);
    afficheDataTest();
}

void Gamel::on_doubleX_valueChanged(double x)
{
    CaptIrCNX *c = modele->getCaptIr(captIrIndex);
    c->setX(x);
    double px = changeRepX(c->getX());
    double py = changeRepY(c->getY());
    tabItems[captIrIndex]->setPos(px, py);
    focusItem->blockSignals(true);
    focusItem->setPos(px, py);
    focusItem->blockSignals(false);
}

void Gamel::on_doubleY_valueChanged(double y)
{
    CaptIrCNX *c = modele->getCaptIr(captIrIndex);
    c->setY(y);
    double px = changeRepX(c->getX());
    double py = changeRepY(c->getY());
    tabItems[captIrIndex]->setPos(px, py);
    focusItem->blockSignals(true);
    focusItem->setPos(px, py);
    focusItem->blockSignals(false);
}

void Gamel::on_doubleH_valueChanged(double h)
{
    modele->getCaptIr(captIrIndex)->setH(h);
    afficheDataTest();
}

void Gamel::on_cnyPin_currentIndexChanged(int index)
{
    int pinIndex = ui->cnyPin->itemData(index).toInt();
    modele->captIrConnecte(captIrIndex, pinIndex);
    changePinsInUse();
    ui->schemaPinCny->setText(ui->cnyPin->currentText());
}

void Gamel::moveCapteur()
{
    double px = focusItem->x(), py = focusItem->y();
    double x, y;
    x = changeInvRepX(px);
    y = changeInvRepY(py);
    ui->doubleX->blockSignals(true);
    ui->doubleX->setValue(x);
    ui->doubleX->blockSignals(false);
    ui->doubleY->blockSignals(true);
    ui->doubleY->setValue(y);
    ui->doubleY->blockSignals(false);
    CaptIrCNX *c = modele->getCaptIr(captIrIndex);
    c->setX(x);
    c->setY(y);
    tabItems[captIrIndex]->setPos(px, py);
}

void Gamel::on_checkBoxTestBlanc_toggled(bool checked)
{
    if (checked) {
        tabItems[captIrIndex]->setPixmap(QPixmap(":/images/cny70blanc.png"));
        modele->getCaptIr(captIrIndex)->forceSurLigne();
    } else {
        tabItems[captIrIndex]->setPixmap(QPixmap(":/images/cny70bleu.png"));
        modele->getCaptIr(captIrIndex)->forceSurMoquette();
    }
    afficheDataTest();
}

void Gamel::afficheDataTest()
{
    CaptIrCNX *c = modele->getCaptIr(captIrIndex);

    ui->labelV0->setText(QLocale().toString(c->getV0(), 'f', 3)+QString(" V"));
    ui->labelVs->setText(QLocale().toString(c->getVs(), 'f', 3)+QString(" V"));
}

void Gamel::fdcChange()
{
    fdc->blockSignals(true);
    fdc->setChecked(modele->getFdc().isActif());
    fdc->blockSignals(false);
}

void Gamel::on_doubleMoquette_valueChanged(double val)
{
    CaptIrCNX::setRefMoquette(val);
}

void Gamel::on_doubleScotch_valueChanged(double val)
{
    CaptIrCNX::setRefScotch(val);
}

void Gamel::on_doubleVoie_valueChanged(double val)
{
    modele->setVoie(val);
}

void Gamel::on_doubleVbat_valueChanged(double val)
{
    modele->setVbat(val);
}

void Gamel::on_doubleMasse_valueChanged(double val)
{
    modele->setMasse(val);
}

void Gamel::on_doubleKroul_valueChanged(double val)
{
    modele->setKroul(val);
}

void Gamel::on_doubleDiametreRoue_valueChanged(double val)
{
    modele->setRayon(val / 2.0);
}

void Gamel::on_doubleRapport_valueChanged(double val)
{
    modele->setReduc(val);
}

void Gamel::on_doubleRendement_valueChanged(double val)
{
    modele->setRendement(val / 100.0);
}

void Gamel::on_doubleVdiode_valueChanged(double val)
{
    modele->setVf(val);
}

void Gamel::on_doubleRdson_valueChanged(double val)
{
    modele->setRdson(val);
}

void Gamel::on_doubleRmotG_valueChanged(double val)
{
    modele->getMoteurG()->setRes(val);
    if (!ui->toolButtonDiffMot->isChecked()) {
        modele->getMoteurD()->setRes(val);
        ui->doubleRmotD->blockSignals(true);
        ui->doubleRmotD->setValue(modele->getMoteurD()->getRes());
        ui->doubleRmotD->blockSignals(false);
    }
}

void Gamel::on_doubleLmotG_valueChanged(double val)
{
    val *= 1.0e-3;
    modele->getMoteurG()->setInduc(val);
    if (!ui->toolButtonDiffMot->isChecked()) {
        modele->getMoteurD()->setInduc(val);
        ui->doubleLmotD->blockSignals(true);
        ui->doubleLmotD->setValue(modele->getMoteurD()->getInduc() * 1.0e3);
        ui->doubleLmotD->blockSignals(false);
    }
}

void Gamel::on_doubleKphiG_valueChanged(double val)
{
    modele->getMoteurG()->setKphi(val);
    if (!ui->toolButtonDiffMot->isChecked()) {
        modele->getMoteurD()->setKphi(val);
        ui->doubleKphiD->blockSignals(true);
        ui->doubleKphiD->setValue(modele->getMoteurD()->getKphi());
        ui->doubleKphiD->blockSignals(false);
    }
}

void Gamel::on_doubleInertieG_valueChanged(double val)
{
    modele->getMoteurG()->setInertie(val);
    if (!ui->toolButtonDiffMot->isChecked()) {
        modele->getMoteurD()->setInertie(val);
        ui->doubleInertieD->blockSignals(true);
        ui->doubleInertieD->setValue(modele->getMoteurD()->getInertie());
        ui->doubleInertieD->blockSignals(false);
    }
}

void Gamel::on_doubleI0motG_valueChanged(double val)
{
    modele->getMoteurG()->setI0(val);
    if (!ui->toolButtonDiffMot->isChecked()) {
        modele->getMoteurD()->setI0(val);
        ui->doubleI0motD->blockSignals(true);
        ui->doubleI0motD->setValue(modele->getMoteurD()->getI0());
        ui->doubleI0motD->blockSignals(false);
    }
}

void Gamel::on_doubleRmotD_valueChanged(double val)
{
    modele->getMoteurD()->setRes(val);
}

void Gamel::on_doubleLmotD_valueChanged(double val)
{
    modele->getMoteurD()->setInduc(val * 1.0e-3);
}

void Gamel::on_doubleKphiD_valueChanged(double val)
{
    modele->getMoteurD()->setKphi(val);
}

void Gamel::on_doubleInertieD_valueChanged(double val)
{
    modele->getMoteurD()->setInertie(val);
}

void Gamel::on_doubleI0motD_valueChanged(double val)
{
    modele->getMoteurD()->setI0(val);
}

void Gamel::on_toolButtonDiffMot_toggled(bool checked)
{
    if (!checked) {
        modele->getMoteurD()->setRes(modele->getMoteurG()->getRes());
        modele->getMoteurD()->setInduc(modele->getMoteurG()->getInduc());
        modele->getMoteurD()->setKphi(modele->getMoteurG()->getKphi());
        modele->getMoteurD()->setInertie(modele->getMoteurG()->getInertie());
        modele->getMoteurD()->setI0(modele->getMoteurG()->getI0());
        viewModele();
    }
}

void Gamel::fdcClick(bool on)
{
    if (on) {
        fdc->blockSignals(true);
        fdc->setChecked(false);
        fdc->blockSignals(false);
    }
}

void Gamel::appuiFdc()
{
    fdc->setDown(true);
}

void Gamel::relacheFdc()
{
    fdc->setDown(false);
}

void Gamel::jackToggle()
{
    ui->jack->toggle();
}
