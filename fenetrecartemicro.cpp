#include "fenetrecartemicro.h"
#include "ui_fenetrecartemicro.h"

#include <QFileDialog>
#include <QPushButton>

FenetreCarteMicro::FenetreCarteMicro(Modele &m, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::FenetreCarteMicro),
    lcd(m.getLcd())
{
    ui->setupUi(this);

    modele = &m;

    connect(ui->bp0, SIGNAL(pressed()), modele, SLOT(appuyeBP0()));
    connect(ui->bp1, SIGNAL(pressed()), modele, SLOT(appuyeBP1()));
    connect(ui->bp2, SIGNAL(pressed()), modele, SLOT(appuyeBP2()));
    connect(ui->bp0, SIGNAL(released()), modele, SLOT(relacheBP0()));
    connect(ui->bp1, SIGNAL(released()), modele, SLOT(relacheBP1()));
    connect(ui->bp2, SIGNAL(released()), modele, SLOT(relacheBP2()));

    connect(ui->reset, SIGNAL(pressed()), modele, SLOT(appuyeReset()));
    connect(ui->reset, SIGNAL(released()), modele, SLOT(relacheReset()));

    connect(ui->an0SpinBox, SIGNAL(valueChanged(double)), modele, SLOT(setPotAN0Value(double)));

    connect(&modele->getLedA6(), SIGNAL(setCourant(double)), ui->ledA6, SLOT(setCourant(double)));

    connect(ui->contraste, SIGNAL(valueChanged(int)), &modele->getLcd(), SLOT(setContraste(int)));
    ui->lcd->setScene(lcd.getScene());
}

FenetreCarteMicro::~FenetreCarteMicro()
{
    delete ui;
}

void FenetreCarteMicro::on_an0SpinBox_valueChanged(double value)
{
    ui->an0Dial->setValue(value*1000+0.5);
}

void FenetreCarteMicro::on_an0Dial_valueChanged(int value)
{
    ui->an0SpinBox->setValue(value/1000.0);
}

void FenetreCarteMicro::on_checkBoxMaintienBP0_clicked(bool checked)
{
    ui->bp0->setDisabled(checked);
    if (checked) modele->appuyeBP0(); else modele->relacheBP0();
}

void FenetreCarteMicro::on_checkBoxMaintienBP1_clicked(bool checked)
{
    ui->bp1->setDisabled(checked);
    if (checked) modele->appuyeBP1(); else modele->relacheBP1();
}

void FenetreCarteMicro::on_checkBoxMaintienBP2_clicked(bool checked)
{
    ui->bp2->setDisabled(checked);
    if (checked) modele->appuyeBP2(); else modele->relacheBP2();
}

void FenetreCarteMicro::on_boutonPower_toggled(bool checked)
{
    if (checked) modele->powerOn(); else modele->powerOff();
    ui->ledPower->setChecked(checked);
}

void FenetreCarteMicro::on_pushButtonOuvrir_clicked()
{
#ifdef Q_OS_WASM
    auto fileContentReady = [this](const QString &fichier, const QByteArray &fileContent) {
        if (fichier.isEmpty()) {
            // No file was selected
        } else {
            // Use fichier and fileContent
            ui->fichier->setText(fichier);
            QByteArray ba = fichier.toLatin1();
            QString sandboxFile = "tmp.hex";
            QFile file(sandboxFile);
            file.open(QIODeviceBase::WriteOnly);
            file.write(fileContent);
            file.close();
            modele->getProcesseur().readFromHex(sandboxFile.toLatin1().data());
//            ui->pushButtonRefresh->setEnabled(true);
//            emit refreshEnabled(true);
        }
    };
    QFileDialog::getOpenFileContent(tr("Fichier HEX (*.hex)"),  fileContentReady);
#else
    QString fichier = QFileDialog::getOpenFileName(0, tr("Choisir un fichier"), ui->fichier->text(), tr("Fichier HEX (*.hex)"));
    if (!fichier.isEmpty()) {
        ui->fichier->setText(fichier);
        QByteArray ba = fichier.toLatin1();
        char *c_str2 = ba.data();
        modele->getProcesseur().readFromHex(c_str2);
        ui->pushButtonRefresh->setEnabled(true);
        emit refreshEnabled(true);
    }
#endif
}

void FenetreCarteMicro::on_pushButtonRefresh_clicked()
{
    QByteArray ba = ui->fichier->text().toLatin1();
    char *c_str2 = ba.data();
    modele->getProcesseur().readFromHex(c_str2);
}

void FenetreCarteMicro::set_bp0Pressed()
{
    ui->bp0->setDown(true);
    modele->appuyeBP0();
}

void FenetreCarteMicro::set_bp0Released()
{
    ui->bp0->setDown(false);
    modele->relacheBP0();
}

void FenetreCarteMicro::set_bp1Pressed()
{
    ui->bp1->setDown(true);
    modele->appuyeBP1();
}

void FenetreCarteMicro::set_bp1Released()
{
    ui->bp1->setDown(false);
    modele->relacheBP1();
}

void FenetreCarteMicro::set_bp2Pressed()
{
    ui->bp2->setDown(true);
    modele->appuyeBP2();
}

void FenetreCarteMicro::set_bp2Released()
{
    ui->bp2->setDown(false);
    modele->relacheBP2();
}

void FenetreCarteMicro::set_resetPressed()
{
    ui->reset->setDown(true);
    modele->appuyeReset();
}

void FenetreCarteMicro::set_resetReleased()
{
    ui->reset->setDown(false);
    modele->relacheReset();
}

void FenetreCarteMicro::augmenteAn0()
{
    ui->an0SpinBox->setValue(ui->an0SpinBox->value()+10*ui->an0SpinBox->singleStep());
}

void FenetreCarteMicro::diminueAn0()
{
    ui->an0SpinBox->setValue(ui->an0SpinBox->value()-10*ui->an0SpinBox->singleStep());
}
