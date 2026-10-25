#ifndef BATTERIE_H
#define BATTERIE_H

// ============================================================
// Batterie d'alimentation de la gamelle (Yuasa NP1.2-12 : 12 V / 1,2 Ah, plomb-acide AGM).
// Voir docs/etude-batterie.md. Trois modes :
//   TENSION_CONSTANTE    : v = vbat ("tension de batterie"), Rs inactive (defaut) ;
//   TENSION_CONSTANTE_RS : v = vbat - Rs·i (vbat = tension a vide), Rs reglable (defaut 110 mOhms) ;
//   MODE_BATTERIE        : tension a vide E(SOC) + Rs, decharge integree au temps reel ;
//                          SOC coupe a 0 % => poweroff + signal batterieVide().
// ============================================================

class Batterie
{
public :
    enum Mode { TENSION_CONSTANTE = 0, TENSION_CONSTANTE_RS = 1, MODE_BATTERIE = 2 };

    // Courant consomme par la carte ESP32 (core + peripheriques) quand elle est sous tension
    static constexpr double COURANT_CARTE = 0.070;   // A (70 mA, voir l'etude)

    // Capacite nominale de la batterie (NP1.2-12 au regime 20 h ; l'effet de debit
    // de Peukert n'est pas modelise — cf. etude, ajustement possible par essai)
    static constexpr double CAPACITE = 1.2;          // Ah

    // Polyligne E(SOC) : preset plomb-acide NP1.2-12 au repos (SOC % decroissant,
    // tension a vide en V — calage sur essai reel prevu en validation)
    static constexpr int NB_SOC = 5;
    static constexpr double SOC_TABLE[NB_SOC] = { 100.0, 75.0, 50.0, 25.0, 0.0 };   // %
    static constexpr double E_TABLE[NB_SOC]   = { 13.0, 12.7, 12.4, 12.0, 11.7 };   // V a vide

    // Tension a vide E(SOC) (V) : interpolation lineaire entre les points de la polyligne
    static double tensionVide(double soc) {
        for (int i = 1; i < NB_SOC; i++) {
            if (soc >= SOC_TABLE[i]) {
                double t = (SOC_TABLE[i-1] - SOC_TABLE[i]) > 1e-12
                         ? (soc - SOC_TABLE[i]) / (SOC_TABLE[i-1] - SOC_TABLE[i]) : 0.0;
                if (t > 1.0) t = 1.0;   // soc > 100 % : borne au point haut
                return E_TABLE[i] + t * (E_TABLE[i-1] - E_TABLE[i]);
            }
        }
        return E_TABLE[NB_SOC-1];   // soc < 0 : point bas (cas theorique, soc borne a 0)
    }

    Batterie() {  mode = TENSION_CONSTANTE;  rs = 0.110;  soc = 100.0;  courant = 0.0;  tension = 0.0;  }

    Mode getMode() const {  return mode;  }
    void setMode(Mode m) {  mode = m;  }
    double getRs() const {  return rs;  }
    void setRs(double r) {  rs = (r < 0.0) ? 0.0 : r;  }
    double getSoc() const {  return soc;  }   // charge restante (0..100 %)
    void setSoc(double s) {  if (s < 0.0) s = 0.0;  if (s > 100.0) s = 100.0;  soc = s;  }
    double getCourant() const {  return courant;  }   // dernier courant tire de la batterie (A)
    double getTension() const {  return tension;  }   // derniere tension aux bornes calculee (V)

    // Recalcule la tension aux bornes de la batterie pour un courant total tire iTot
    // pendant dt (secondes), la memorise (affichage de l'onglet Batterie) et la renvoie
    // (tension appliquee aux hacheurs). dt = 0 pour un rafraichissement sans decharge
    // (reglages de l'UI, mise a jour de l'affichage au repos).
    double evalue(double vbat, double iTot, double dt) {
        courant = iTot;
        switch (mode) {
        case TENSION_CONSTANTE_RS :
            // Chute dans la resistance serie ; la Rs ne peut pas inverser la polarite
            tension = vbat - rs * iTot;
            if (tension < 0.0) tension = 0.0;
            break;
        case MODE_BATTERIE :
            // Decharge integree au temps reel (sans acceleration du temps) : le SOC
            // baisse avec la charge retiree, et la tension a vide suit E(SOC)
            if (dt > 0.0) {
                soc -= 100.0 * iTot * dt / (CAPACITE * 3600.0);
                if (soc < 0.0) soc = 0.0;
            }
            tension = tensionVide(soc) - rs * iTot;
            if (tension < 0.0) tension = 0.0;
            break;
        default :   // TENSION_CONSTANTE
            tension = vbat;
            break;
        }
        return tension;
    }

private :
    Mode mode;
    double rs;          // resistance serie (Ohms), defaut 110 mOhms
    double soc;         // charge restante (0..100 %), defaut 100 %
    double courant;     // dernier courant total tire (A)
    double tension;     // derniere tension aux bornes calculee (V)
};

#endif // BATTERIE_H
