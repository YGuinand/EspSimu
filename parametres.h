#ifndef PARAMETRES_H
#define PARAMETRES_H

#include <QString>

// Fichier d'initialisation : espsimu.ini, place a cote de l'executable
// (application portable). Les parametres sauvegardes en cours d'utilisation
// y sont relus au demarrage de l'application ; le fichier est cree a la
// premiere sauvegarde et peut etre consulte/edite par l'utilisateur.

class Parametres
{
public:
    // Dernier firmware .bin charge (chemin absolu) ; chaine vide si aucun.
    static void setFirmwareRecent(const QString &fichier);
    static QString getFirmwareRecent();

    // Derniere gamelle .gml chargee (chemin absolu) ; chaine vide si aucune.
    static void setGamelleRecent(const QString &fichier);
    static QString getGamelleRecent();

    // Derniere piste .pis chargee (chemin absolu) ; chaine vide si aucune.
    static void setPisteRecent(const QString &fichier);
    static QString getPisteRecent();

private:
    // Cle "Section/dernier" du fichier d'initialisation : une section par
    // type de donnee.
    static void enregistrer(const QString &cle, const QString &fichier);
    static QString lire(const QString &cle);
};

#endif // PARAMETRES_H
