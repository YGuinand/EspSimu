#include "parametres.h"

#include <QCoreApplication>
#include <QSettings>

// Chemin du fichier d'initialisation : a cote de l'executable, comme la
// bibliotheque QEMU 3rdparty, pour garder l'application portable.
static QString cheminIni()
{
    return QCoreApplication::applicationDirPath() + "/espsimu.ini";
}

void Parametres::enregistrer(const QString &cle, const QString &fichier)
{
    QSettings paramIni(cheminIni(), QSettings::IniFormat);
    paramIni.setValue(cle, fichier);   // ecrit au retour de la fonction
}

QString Parametres::lire(const QString &cle)
{
    QSettings paramIni(cheminIni(), QSettings::IniFormat);
    return paramIni.value(cle).toString();
}

void Parametres::setFirmwareRecent(const QString &fichier)
{
    enregistrer("Firmware/dernier", fichier);
}

QString Parametres::getFirmwareRecent()
{
    return lire("Firmware/dernier");
}

void Parametres::setGamelleRecent(const QString &fichier)
{
    enregistrer("Gamelle/dernier", fichier);
}

QString Parametres::getGamelleRecent()
{
    return lire("Gamelle/dernier");
}

void Parametres::setPisteRecent(const QString &fichier)
{
    enregistrer("Piste/dernier", fichier);
}

QString Parametres::getPisteRecent()
{
    return lire("Piste/dernier");
}

// Valeur booleenne : pas de passerelle par enregistrer/lire (dedies aux
// chemins de fichiers). Defaut selon la plateforme : boite fichier du
// systeme sous Windows (custom Qt pas plus jolie que la native, look
// Windows attendu par les etudiants), boite simplifiee dediee aux
// firmwares sous Linux.
void Parametres::setDialogueSysteme(bool systeme)
{
    QSettings paramIni(cheminIni(), QSettings::IniFormat);
    paramIni.setValue("Firmware/dialogueSysteme", systeme);   // ecrit au retour de la fonction
}

bool Parametres::getDialogueSysteme()
{
#ifdef Q_OS_WIN
    static const bool defaut = true;    // Windows : boite systeme par defaut
#else
    static const bool defaut = false;   // Linux : boite simplifiee par defaut
#endif
    QSettings paramIni(cheminIni(), QSettings::IniFormat);
    return paramIni.value("Firmware/dialogueSysteme", defaut).toBool();
}
