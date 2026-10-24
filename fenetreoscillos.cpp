#include "fenetreoscillos.h"
#include "ui_fenetreoscillos.h"

FenetreOscillos::FenetreOscillos(Modele &m, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::FenetreOscillos)
{
    ui->setupUi(this);

    modele = &m;

    // Sources de signal pour les deux oscilloscopes (GPIO brute)
    srcGpio2  = [this](double &rapport, double &frequence) {
        return modele->etatPwmGpio(2,  rapport, frequence); };
    srcGpio4  = [this](double &rapport, double &frequence) {
        return modele->etatPwmGpio(4,  rapport, frequence); };
    srcGpio26 = [this](double &rapport, double &frequence) {
        return modele->etatPwmGpio(26, rapport, frequence); };
    srcGpio27 = [this](double &rapport, double &frequence) {
        return modele->etatPwmGpio(27, rapport, frequence); };

    // Liste identique pour les deux oscilloscopes : IO2, IO4, IO26, IO27
    QStringList gpioList;
    gpioList << "IO2" << "IO4" << "IO26" << "IO27";

    oscillo1 = new Oscillo(ui->graphicsView1);
    oscillo1->setSource(srcGpio26);   // defaut : IO26 en haut
    ui->comboBox_1->addItems(gpioList);
    ui->comboBox_1->setCurrentIndex(2); // IO26

    oscillo2 = new Oscillo(ui->graphicsView2);
    oscillo2->setSource(srcGpio27);   // defaut : IO27 en bas
    ui->comboBox_2->addItems(gpioList);
    ui->comboBox_2->setCurrentIndex(3); // IO27

    // Basculer la source selon la selection dans chaque liste deroulante
    connect(ui->comboBox_1, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
                switch (idx) {
                    case 0: oscillo1->setSource(srcGpio2);  break;
                    case 1: oscillo1->setSource(srcGpio4);  break;
                    case 2: oscillo1->setSource(srcGpio26); break;
                    default: oscillo1->setSource(srcGpio27); break;
                }
            });
    connect(ui->comboBox_2, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
                switch (idx) {
                    case 0: oscillo2->setSource(srcGpio2);  break;
                    case 1: oscillo2->setSource(srcGpio4);  break;
                    case 2: oscillo2->setSource(srcGpio26); break;
                    default: oscillo2->setSource(srcGpio27); break;
                }
            });

    // Rafraichissement de la trace (10 Hz)
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &FenetreOscillos::miseAJour);
    timer->start(100);
}

FenetreOscillos::~FenetreOscillos()
{
    delete ui;
    delete oscillo1;
    delete oscillo2;
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

void FenetreOscillos::miseAJour()
{
    if (!isVisible()) return;
    oscillo1->rafraichir();
    oscillo2->rafraichir();
}
