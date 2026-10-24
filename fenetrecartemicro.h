#ifndef FENETRECARTEMICRO_H
#define FENETRECARTEMICRO_H

#include "lcdscreen.h"
#include "modele.h"

#include <QFrame>
#include <QLabel>
#include <QCloseEvent>

namespace Ui {
    class FenetreCarteMicro;
}

class FenetreCarteMicro : public QFrame
{
    Q_OBJECT

public:
    explicit FenetreCarteMicro(Modele &m, QWidget *parent = 0);
    ~FenetreCarteMicro();

    // Fichier d'initialisation (voir parametres.h) : au demarrage, charger le
    // dernier firmware utilise s'il existe encore (charge, pas demarre :
    // l'utilisateur actionne ensuite Power pour lancer l'ESP32).
    void chargeFirmwareRecent();

private:
    Ui::FenetreCarteMicro *ui;

    Modele *modele;

    LcdScreenView lcd;

    // UI a jour apres un firmware accepte : chemin affiche, bouton/menu
    // Recharger actives (utilise par Ouvrir, Recharger et chargeFirmwareRecent).
    void afficheFirmwareCharge(const QString &fichier);

private slots:
    void on_an0Dial_valueChanged(int value);
    void on_an0SpinBox_valueChanged(double );
    void on_checkBoxMaintienBP0_clicked(bool checked);
    void on_checkBoxMaintienBP1_clicked(bool checked);
    void on_checkBoxMaintienBP2_clicked(bool checked);
    void on_boutonPower_toggled(bool checked);
    void on_pushButtonOuvrir_clicked();
    void on_pushButtonRefresh_clicked();

signals :
    void estCache(bool);
    void refreshEnabled(bool);

protected :
    virtual void closeEvent (QCloseEvent* event) {  event->ignore();  if (parentWidget()) parentWidget()->hide(); else hide();  emit estCache(true); }

public:
    void set_bp0Pressed();
    void set_bp0Released();
    void set_bp1Pressed();
    void set_bp1Released();
    void set_bp2Pressed();
    void set_bp2Released();
    void set_resetPressed();
    void set_resetReleased();
    void augmenteAn0();
    void diminueAn0();
};

#endif // FENETRECARTEMICRO_H
