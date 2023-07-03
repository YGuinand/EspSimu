#include "fenetrepiste.h"
#include "ui_fenetrepiste.h"

#include "piste.h"
#include "modele.h"

#include <QFileDialog>

FenetrePiste::FenetrePiste(Modele &m, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::FenetrePiste), modele(&m),
    scene(modele->xMin, modele->yMin, modele->xMax-modele->xMin, modele->yMax-modele->yMin), fichier("piste001.pis")
{
    ui->setupUi(this);

    scene.setBackgroundBrush(Qt::gray);

    scene.addRect(modele->xMin, modele->yMin, modele->xMax-modele->xMin, modele->yMax-modele->yMin, Qt::NoPen, Qt::blue);

    modele->getPiste().openData(":/piste001.pis");
    modele->getPiste().ajouterDansScene(scene);

    gamel = new GamelItem(modele->xMin, modele->yMin, modele->xMax, modele->yMax);
    gamel->setZValue(1.0);
    scene.addItem(gamel);
    gamel->hide();

    ui->graphicsView->scale(64.0/240.0,64.0/240.0);
    ui->graphicsView->setScene(&scene);

    double zoom0 = 0.22876792;
    ui->graphicsView->scale(zoom0, zoom0);

    connect(gamel, SIGNAL(moved(qreal,qreal,qreal)), modele, SLOT(gamelSurPiste(qreal,qreal,qreal)));
    connect(modele, SIGNAL(deplaceGamel(double,double,double)), gamel, SLOT(deplacement(double,double,double)));

    label[0] = ui->label1;
    label[1] = ui->label2;
    label[2] = ui->label3;
    dbl[0] = ui->doubleSpinBox1;
    dbl[1] = ui->doubleSpinBox2;
    dbl[2] = ui->doubleSpinBox3;
    numEdit = 0;
    editPiste();
    ui->widgetEditPiste->hide();
    modele->getPiste().noFocus();
}

FenetrePiste::~FenetrePiste()
{
    delete ui;
}

void FenetrePiste::drawPiste()
{
    modele->getPiste().ajouterDansScene(scene);
    numEdit = 0;
    editPiste();
    if (!ui->toolButtonEdit->isChecked()) modele->getPiste().noFocus();
}

void FenetrePiste::on_toolButtonZoomP_clicked()
{
    double z=10.0/9.0;
    ui->graphicsView->scale(z, z);
}

void FenetrePiste::on_toolButtonZoomM_clicked()
{
    ui->graphicsView->scale(0.9, 0.9);
}

void FenetrePiste::on_toolButtonClk_clicked()
{
    gamel->setRotation(gamel->rotation()+10);
    gamel->signalMoved();
}

void FenetrePiste::on_toolButtonAntiClk_clicked()
{
    gamel->setRotation(gamel->rotation()-10);
    gamel->signalMoved();
}

void FenetrePiste::on_toolButtonPoseGamel_toggled(bool checked)
{
    gamel->setVisible(checked);
    if (checked) {
        gamel->setRotation(0);
        gamel->setPos(0,0);
//        gamel->signalMoved();
    }
}

void FenetrePiste::editPiste()
{
    Piste *p = &modele->getPiste();
    unsigned int taille = p->size();

    if (numEdit >= taille) {
        numEdit = taille;
        if (numEdit != 0) numEdit--;
    }
    ui->toolButtonPrev->setDisabled(numEdit == 0);
    ui->toolButtonNext->setDisabled(numEdit+1 >= taille);
    ui->labelNumero->setText(QString().setNum(numEdit));
    if (taille == 0) {
        ui->comboBoxTypes->hide();
        ui->label1->hide();
        ui->doubleSpinBox1->hide();
        ui->label2->hide();
        ui->doubleSpinBox2->hide();
        ui->label3->hide();
        ui->doubleSpinBox3->hide();
        ui->toolButtonSuppr->setDisabled(true);
    } else {
        Portion *q = p->getElt(numEdit);
        ui->comboBoxTypes->show();
        ui->comboBoxTypes->blockSignals(true);
        ui->comboBoxTypes->setCurrentIndex(q->typeIndex());
        ui->comboBoxTypes->blockSignals(false);
        int i;
        for (i=q->nbArg(); i<3; i++) {
            label[i]->hide();
            dbl[i]->hide();
        }
        for (i=0; i<q->nbArg(); i++) {
            label[i]->setText(q->nameArg(i));
            dbl[i]->blockSignals(true);
            dbl[i]->setMaximum(q->maxArg(i));
            dbl[i]->setMinimum(q->minArg(i));
            dbl[i]->setSuffix(q->suffixArg(i));
            dbl[i]->setDecimals(q->decimalsArg(i));
            dbl[i]->setValue(q->valArg(i));
            dbl[i]->blockSignals(false);
            label[i]->show();
            dbl[i]->show();
        }
        ui->toolButtonSuppr->setEnabled(true);
    }
    p->setFocus(numEdit);
}

void FenetrePiste::on_toolButtonNext_clicked()
{
    numEdit++;
    editPiste();
}

void FenetrePiste::on_toolButtonPrev_clicked()
{
    numEdit--;
    editPiste();
}

void FenetrePiste::on_toolButtonNew_clicked()
{
    if (numEdit < modele->getPiste().size()) numEdit++;
    modele->getPiste().eltInsert(numEdit, ui->comboBoxTypes->currentIndex());
    editPiste();
}

void FenetrePiste::on_toolButtonSuppr_clicked()
{
    modele->getPiste().eltSupprime(numEdit);
    editPiste();
}

void FenetrePiste::on_comboBoxTypes_currentIndexChanged(int index)
{
    modele->getPiste().eltRemplace(numEdit, index);
    editPiste();
}

void FenetrePiste::on_doubleSpinBox1_valueChanged(double val)
{
    modele->getPiste().eltModifArg(numEdit, 0, val);
}

void FenetrePiste::on_doubleSpinBox2_valueChanged(double val)
{
    modele->getPiste().eltModifArg(numEdit, 1, val);
}

void FenetrePiste::on_doubleSpinBox3_valueChanged(double val)
{
    modele->getPiste().eltModifArg(numEdit, 2, val);
}

void FenetrePiste::on_toolButtonOpen_clicked()
{
#ifdef Q_OS_WASM
    auto fileContentReady = [this](const QString &fichier, const QByteArray &fileContent) {
        if (fichier.isEmpty()) {
            // No file was selected
        } else {
            // Use fichier and fileContent
            QString sandboxFile = "tmp.pis";
            QFile file(sandboxFile);
            file.open(QIODeviceBase::WriteOnly);
            file.write(fileContent);
            file.close();
            modele->getPiste().openData(sandboxFile);
            drawPiste();
        }
    };
    QFileDialog::getOpenFileContent(tr("Fichier PIS (*.pis)"),  fileContentReady);
#else
    QString f = QFileDialog::getOpenFileName(0, QString(tr("Charger les données d'une piste")), fichier, tr("Fichier PIS (*.pis)"));
    if (!f.isEmpty()) {
        fichier = f;
        modele->getPiste().openData(fichier);
        drawPiste();
    }
#endif
}

void FenetrePiste::on_toolButtonSave_clicked()
{
#ifdef Q_OS_WASM
    QString sandboxFile = "tmpSave.pis";
    modele->getPiste().saveData(sandboxFile);
    QFile file(sandboxFile);
    file.open(QIODeviceBase::ReadOnly);
    QByteArray data = file.readAll();
    file.close();
    QFileDialog::saveFileContent(data, "pisteDatas.pis");
#else
    QString f = QFileDialog::getSaveFileName(0, QString(tr("Sauvegarder les données d'une piste")), fichier, tr("Fichier PIS (*.pis)"));
    if (!f.isEmpty()) {
        fichier = f;
        modele->getPiste().saveData(fichier);
    }
#endif
}

void FenetrePiste::on_toolButtonEdit_toggled(bool checked)
{
    ui->widgetEditPiste->setVisible(checked);
    if (checked) {
        modele->getPiste().setFocus(numEdit);
    } else {
        modele->getPiste().noFocus();
    }
}

void FenetrePiste::on_toolButtonZoomFit_clicked()
{
    ui->graphicsView->fitInView(modele->xMin, modele->yMin, modele->xMax-modele->xMin, modele->yMax-modele->yMin, Qt::KeepAspectRatio);
}
