#include "fenetreoscillos.h"
#include "ui_fenetreoscillos.h"

FenetreOscillos::FenetreOscillos(Modele &m, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::FenetreOscillos)
{
    ui->setupUi(this);

    modele = &m;

//    ledPwm1.setGraphics(&modele->getProcesseur(), ui->ledPwm1, ":/images/led_verte_on.png",":/images/led_verte_off.png");
//    modele->getProcesseur().getPortC()->connecte(2, &ledPwm1);
    ui->ledPwm1->hide();

//    ledPwm2.setGraphics(&modele->getProcesseur(), ui->ledPwm2, ":/images/led_verte_on.png",":/images/led_verte_off.png");
//    modele->getProcesseur().getPortC()->connecte(1, &ledPwm2);
    ui->ledPwm2->hide();

    oscillo1 = new Oscillo(&modele->getProcesseur(), ui->qwtPlot_1);
//    oscillo1->observer(&ledPwm1);
    oscillo1->observer(modele->getMoteurD(), 0);
    ui->comboBox_1->addItem(modele->getMoteurD()->getNameParam(0));

    oscillo2 = new Oscillo(&modele->getProcesseur(), ui->qwtPlot_2);
//    oscillo2->observer(&ledPwm2);
    oscillo2->observer(modele->getMoteurG(), 0);
    ui->comboBox_2->addItem(modele->getMoteurG()->getNameParam(0));

    connect(modele, SIGNAL(cnxMotChange()), this, SLOT(cnxChange()));
}

FenetreOscillos::~FenetreOscillos()
{
    delete ui;
    delete oscillo1;
    delete oscillo2;
}

void FenetreOscillos::cnxChange()
{
    ui->comboBox_1->setItemText(0, modele->getMoteurD()->getNameParam(0));
    ui->comboBox_2->setItemText(0, modele->getMoteurG()->getNameParam(0));
}

void FenetreOscillos::on_baseTpsOscillo1_valueChanged(int value)
{
    oscillo1->setBaseTemps(value);
    if (!ui->checkBoxBaseTps2->isChecked()) ui->baseTpsOscillo2->setValue(value);
}

void FenetreOscillos::on_baseTpsOscillo2_valueChanged(int value)
{
    oscillo2->setBaseTemps(value);
}

void FenetreOscillos::on_checkBoxBaseTps2_clicked(bool checked)
{
    if (checked) {
        ui->baseTpsOscillo2->setEnabled(true);
        ui->labelBaseTps2->setEnabled(true);
    } else {
        ui->baseTpsOscillo2->setValue(ui->baseTpsOscillo1->value());
        ui->baseTpsOscillo2->setEnabled(false);
        ui->labelBaseTps2->setEnabled(false);
    }
}


void LedPwmCNX::setGraphics(P18f4550 *p, QGraphicsView *gv, QString str_on, QString str_off)
{
    processeur = p;
    if (!gv) return;

    on.setPixmap(QPixmap(str_on));
    on.setPos(0,0);
    on.setScale(1);
    on.setOpacity(0);
    gs.addItem(&on);
    off.setPixmap(QPixmap(str_off));
    off.setPos(0,0);
    off.setScale(1);
    off.setOpacity(1.0);
    gs.addItem(&off);
    gv->setScene(&gs);
}

void LedPwmCNX::modifie(double /* v */)
{
    uint64_t newt = processeur->time();
    double mtsurtau = ((double)(newt-temps)) / 48000.0;
    vs = (vs-vmemo)*exp(-mtsurtau) + vmemo;
    temps = newt;
    vmemo = getVin();
    alpha = vs / 5.0;
    redraw();
}

