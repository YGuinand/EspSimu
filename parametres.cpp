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
