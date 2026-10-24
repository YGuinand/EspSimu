#include "fenetrecartemicro.h"
#include "ui_fenetrecartemicro.h"
#include "parametres.h"

#include <QFileDialog>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QMessageBox>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>

// Filtre dedie a l'ouverture d'un firmware ESP32.
// Entre un QFileSystemModel (qui inclut les entites cachees) et la vue.
// N'accepte que : les dossiers non caches, le dossier cache .pio (PlatformIO),
// et les fichiers .bin. Les autres dossiers/fichiers caches sont masques.
class FirmwareFilterProxyModel : public QSortFilterProxyModel
{
public:
    explicit FirmwareFilterProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent) {}

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
    {
        QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
        if (!idx.isValid())
            return false;
        const QFileInfo info = static_cast<const QFileSystemModel *>(sourceModel())->fileInfo(idx);

        // Fichiers binaires : toujours visibles
        if (info.isFile() && info.suffix() == QLatin1String("bin"))
            return true;
        // Dossiers : visibles s'ils ne sont pas caches
        if (info.isDir() && !info.isHidden())
            return true;
        // Le dossier cache .pio est le seul dossier cache rendu visible
        if (info.isDir() && info.fileName() == QLatin1String(".pio"))
            return true;
        return false;
    }

    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override
    {
        const QFileSystemModel *fsModel =
            static_cast<const QFileSystemModel *>(sourceModel());
        const QFileInfo leftInfo = fsModel->fileInfo(left);
        const QFileInfo rightInfo = fsModel->fileInfo(right);

        const bool leftIsPio = leftInfo.isDir() && leftInfo.fileName() == QLatin1String(".pio");
        const bool rightIsPio = rightInfo.isDir() && rightInfo.fileName() == QLatin1String(".pio");

        // Le dossier .pio passe toujours en premier
        if (leftIsPio && !rightIsPio)
            return true;
        if (!leftIsPio && rightIsPio)
            return false;

        // Ordre alphabétique croissant (insensible à la casse) sur le nom
        return QString::compare(leftInfo.fileName(), rightInfo.fileName(), Qt::CaseInsensitive) < 0;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role == Qt::FontRole) {
            QModelIndex sourceIdx = mapToSource(index);
            if (sourceIdx.isValid()) {
                QFileInfo info = static_cast<const QFileSystemModel *>(sourceModel())->fileInfo(sourceIdx);
                const QString name = info.fileName();
                if ((info.isFile() && (name == QLatin1String("firmware.bin") ||
                                       name == QLatin1String("firmware.factory.bin"))) ||
                    (info.isDir() && (name == QLatin1String("build") || name == QLatin1String(".pio")))) {
                    QFont font;
                    font.setBold(true);
                    return font;
                }
            }
        }
        return QSortFilterProxyModel::data(index, role);
    }
};

static QString choisirFirmware(QWidget *parent, const QString &repertoireDepart)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(QObject::tr("Choisir un firmware"));
    dialog.resize(700, 500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // Modèle source : inclut les entités cachées pour pouvoir voir .pio
    QFileSystemModel *sourceModel = new QFileSystemModel(&dialog);
    sourceModel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files | QDir::Hidden);
    sourceModel->setRootPath(QString());

    // Proxy : n'affiche que les dossiers non cachés, le dossier caché .pio,
    // et les fichiers .bin (les autres dossiers/fichiers cachés sont masqués).
    FirmwareFilterProxyModel *proxy = new FirmwareFilterProxyModel(&dialog);
    proxy->setSourceModel(sourceModel);

    QTreeView *view = new QTreeView(&dialog);
    view->setModel(proxy);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSortingEnabled(true);
    view->sortByColumn(0, Qt::AscendingOrder);
    view->setAnimated(false);
    for (int col = 1; col < proxy->columnCount(); ++col)
        view->hideColumn(col);

    // Barre de navigation : disque + dossier courant + bouton remonter (en haut)
    QHBoxLayout *navLayout = new QHBoxLayout;
    QLabel *label = new QLabel(QObject::tr("Dossier :"));
    QLineEdit *pathEdit = new QLineEdit;
    pathEdit->setReadOnly(true);
    QPushButton *btnUp = new QPushButton(QStringLiteral("\u2191")); // flèche vers le haut
    btnUp->setToolTip(QObject::tr("Remonter au dossier parent"));

    // Selecteur de disque : sous Windows les lecteurs sont des branches
    // paralleles de la racine et QDir::cdUp() s'arrete a la racine du disque ;
    // sans ce selecteur, on reste prisonnier du disque de depart.
    QComboBox *driveBox = new QComboBox(&dialog);
    const QFileInfoList disques = QDir::drives();
    for (const QFileInfo &disque : disques)
        driveBox->addItem(disque.absoluteFilePath());

    navLayout->addWidget(label);
    navLayout->addWidget(driveBox);
    navLayout->addWidget(pathEdit);
    navLayout->addWidget(btnUp);
    layout->addLayout(navLayout);

    layout->addWidget(view);

    QDialogButtonBox *buttons =
        new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);

    QString currentRoot = repertoireDepart;
    if (currentRoot.isEmpty() || !QDir(currentRoot).exists())
        currentRoot = QDir::currentPath();

    auto refreshView = [&]() {
        QModelIndex sourceIdx = sourceModel->index(currentRoot);
        QModelIndex proxyIdx = proxy->mapFromSource(sourceIdx);
        view->setRootIndex(proxyIdx);
        pathEdit->setText(currentRoot);
        if (proxyIdx.isValid()) {
            view->setCurrentIndex(proxyIdx);
            view->scrollTo(proxyIdx);
        }
    };

    // Pre-selection du disque du dossier de depart, avant la connexion
    // (pour ne pas declencher la navigation a l'ouverture).
    for (int i = 0; i < driveBox->count(); ++i) {
        if (currentRoot.startsWith(driveBox->itemText(i)))
            driveBox->setCurrentIndex(i);
    }
    QObject::connect(driveBox, &QComboBox::currentIndexChanged, [&](int idx) {
        if (idx < 0)
            return;
        currentRoot = driveBox->itemText(idx);
        refreshView();
    });

    QString resultat;

    // Double-clic : descendre dans un dossier, ou ouvrir directement un fichier .bin
    QObject::connect(view, &QTreeView::doubleClicked, [&](const QModelIndex &proxyIdx) {
        QModelIndex sourceIdx = proxy->mapToSource(proxyIdx);
        if (!sourceIdx.isValid())
            return;
        QFileInfo info = sourceModel->fileInfo(sourceIdx);
        if (info.isDir()) {
            currentRoot = info.absoluteFilePath();
            refreshView();
        } else if (info.isFile() && info.suffix() == QLatin1String("bin")) {
            resultat = info.absoluteFilePath();
            dialog.accept();
        }
    });

    // Bouton Remonter : aller dans le dossier parent
    QObject::connect(btnUp, &QPushButton::clicked, [&]() {
        QDir d(currentRoot);
        if (d.cdUp()) {
            currentRoot = d.absolutePath();
            refreshView();
        }
    });

    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    refreshView();

    if (dialog.exec() == QDialog::Accepted && resultat.isEmpty()) {
        const QModelIndexList sel = view->selectionModel()->selectedRows();
        if (!sel.isEmpty()) {
            QModelIndex sourceIdx = proxy->mapToSource(sel.first());
            if (!sourceIdx.isValid())
                return QString();
            QFileInfo info = sourceModel->fileInfo(sourceIdx);
            if (info.isFile() && info.suffix() == QLatin1String("bin"))
                resultat = info.absoluteFilePath();
        }
    }
    return resultat;
}

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
    connect(&modele->getLed2(), SIGNAL(setCourant(double)), ui->ledA6_2,  SLOT(setCourant(double)));

    connect(ui->contraste, SIGNAL(valueChanged(int)), &modele->getLcd(), SLOT(setContraste(int)));

    // Connexion du retroeclairage RGB
    connect(modele, &Modele::backlightChanged, &lcd, &LcdScreenView::setBacklightColor);

    ui->lcd->setScene(lcd.getScene());

    ui->ledA6_2->setColor(LedAlpha::JAUNE);
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
    if (checked) {
        // Verifier qu'un firmware est charge
        if (ui->fichier->text().isEmpty()) {
            QMessageBox::warning(this, "EspSimu",
                                 "Veuillez d'abord charger un firmware (.bin) avec le bouton Ouvrir.");
            ui->boutonPower->setChecked(false);
            return;
        }
        if (!modele->isEsp32Running()) {
            modele->startEsp32();
        }
    } else {
        if (modele->isEsp32Running()) {
            modele->stopEsp32();
        }
    }
    ui->ledPower->setChecked(checked);
}
// UI a jour apres un firmware accepte : chemin affiche, bouton/menu
// Recharger actives.
void FenetreCarteMicro::afficheFirmwareCharge(const QString &fichier)
{
    ui->fichier->setText(fichier);
    ui->pushButtonRefresh->setEnabled(true);
    emit refreshEnabled(true);
}

void FenetreCarteMicro::on_pushButtonOuvrir_clicked()
{
    // Dossier de départ : celui du fichier précédemment sélectionné, s'il existe
    QString repertoireDepart = ui->fichier->text();
    if (!repertoireDepart.isEmpty()) {
        QFileInfo info(repertoireDepart);
        if (info.isFile())
            repertoireDepart = info.dir().absolutePath();
    }

    QString fichier = choisirFirmware(this, repertoireDepart);
    if (fichier.isEmpty()) return;

    // Le nom n'est affiche que si le firmware est accepte
    if (modele->loadEsp32Firmware(fichier)) {
        afficheFirmwareCharge(fichier);
        qDebug() << "Firmware ESP32 charge:" << fichier;
        // Fichier d'initialisation : memorise pour le prochain demarrage
        Parametres::setFirmwareRecent(fichier);
    } else {
        // Le backend sait precisement pourquoi : afficher son message apres la
        // regle generale (les deux seuls formats universellement acceptes).
        // RichText post-traite : tout est echappe d'abord (le chemin du detail
        // ne doit jamais etre pris pour du HTML), puis CHAQUE mention de
        // firmware.factory.bin (message general ET detail backend) passe en
        // gras, et les retours a la ligne deviennent <br>.
        const QString detail = modele->getErreurEsp32();
        QString msg = "Firmware ESP32 refuse. Utilisez firmware.bin / firmware.factory.bin "
                      "d'un build PlatformIO (dossier .pio/build/...) ou une image flash "
                      "de 2, 4, 8 ou 16 Mo.";
        if (!detail.isEmpty()) msg += "\n\n" + detail;
        msg = msg.toHtmlEscaped();
        msg.replace("firmware.factory.bin", "<b>firmware.factory.bin</b>");
        msg.replace("\n", "<br>");
        QMessageBox boite(QMessageBox::Warning, "EspSimu", msg, QMessageBox::Ok, this);
        boite.setTextFormat(Qt::RichText);
        boite.exec();
    }
}

void FenetreCarteMicro::on_pushButtonRefresh_clicked()
{
    QString fichier = ui->fichier->text();
    if (fichier.isEmpty()) return;
    // Carte en cours, la reprogrammation + redemarrage sont geres par loadEsp32Firmware
    if (!modele->loadEsp32Firmware(fichier)) {
        const QString detail = modele->getErreurEsp32();
        QMessageBox::warning(this, "EspSimu", detail.isEmpty()
                             ? "Impossible de recharger le firmware sélectionné."
                             : detail);
    } else {
        Parametres::setFirmwareRecent(fichier);
    }
}

// Recharge, au demarrage de l'application, le dernier firmware utilise
// tel qu'il a ete sauvegarde dans espsimu.ini. Ignoré si le fichier a
// disparu ou est refuse par loadEsp32Firmware.
void FenetreCarteMicro::chargeFirmwareRecent()
{
    QString fichier = Parametres::getFirmwareRecent();
    if (fichier.isEmpty() || !QFile::exists(fichier)) return;
    if (!modele->loadEsp32Firmware(fichier)) return;
    afficheFirmwareCharge(fichier);
    qDebug() << "Firmware ESP32 recharge depuis l'ini:" << fichier;
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
