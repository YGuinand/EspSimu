#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include <QString>

// Diagnostic des blocages de la VM QEMU, pour reproduire et mesurer le lag
// aleatoire observe apres un reset (firmware relance, premier tour de loop
// execute, puis temps d'arret aleatoire). Trois hypotheses a discriminer :
//
//   (a) le temps virtuel QEMU cesse d'avancer (main loop / timers QEMU figes)
//       -> echantillonnage periodique de QEMU_CLOCK_VIRTUAL ;
//   (b) le firmware dort en temps virtuel (delay MS sans delta wall equivalent)
//       -> ecart entre les emissions UART mesure en temps mur ET en temps virtuel ;
//   (c) livraison ralentie cote appli Qt (file d'evenements saturee)
//       -> horodatage des emissions UART a la source (thread QEMU) et de
//          leur arrivee dans le thread Qt.
//
// Le tout est active par la variable d'environnement ESPSIMU_DIAG=1.
// Les traces sortent sur stderr via qDebug, marqueur "EspDiag:".

namespace EspDiag {

// A appeler une fois, apres le chargement de la lib QEMU. libHandle permet
// de resoudre qemu_clock_get_ns (horloge virtuelle). Inactif sans ESPSIMU_DIAG=1.
void init(void *libHandle);

// Marqueur d'evenement : horodate (temps mur + temps virtuel) et garde pour
// le dump. Un label commence par "reset" avance l'origine des temps relatifs.
void mark(const char *label);

// Horodatage d'un octet UART a la source (appel depuis le thread QEMU).
void uartSource(int uartId, unsigned char octet);

// Horodatage d'un octet UART a la livraison Qt (thread appli).
void uartLivraison(int uartId);

// Echantillonnage du temps virtuel QEMU (periode 20 ms) et detection des
// periodes ou il cesse d'avancer pendant que le temps mur continue.
void demarrerEchantillonneur();
void arreterEchantillonneur();

// Etat courant, traces des derniers evenements (marqueurs, periodes figees,
// rafales UART de la derniere minute).
bool actif();
void dump(const QString &raison);

} // namespace EspDiag

#endif // DIAGNOSTIC_H
