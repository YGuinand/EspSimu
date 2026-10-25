#ifndef PONTDIV_H
#define PONTDIV_H

// ============================================================
// Pont diviseur de tension branche en sonde sur la batterie (onglet Batterie,
// see. docs/etude-batterie.md) : R1 en serie depuis le plus de la batterie, R2
// vers la masse ; la tension au point milieu v_out = v_bat · R2/(R1+R2) est
// envoyee sur une entree analogique (ADC1) du connecteur J2, pour que le
// firmware mesure la tension de la batterie (comme les CNY70 alimentent J2).
// Non connecte par defaut (j2Pin = -1). Resisteances choisies par
// l'utilisateur (defauts 33 kOhms / 10 kOhms : 2,79 V sous 12 V, 3,09 V sous
// 13,3 V — sous la pleine echelle de l'ADC).
// ============================================================

class PontDiv
{
public :
    // Defauts : 33 kOms / 10 kOhms (ratio 10/43)
    PontDiv() {  r1 = 33000.0;  r2 = 10000.0;  j2Pin = -1;  mvEnvoye = -1;  }

    double getR1() const {  return r1;  }   // resistance haute (depuis la batterie), Ohms
    void setR1(double ohms) {  r1 = (ohms < 1.0) ? 1.0 : ohms;  }
    double getR2() const {  return r2;  }   // resistance basse (vers la masse), Ohms
    void setR2(double ohms) {  r2 = (ohms < 1.0) ? 1.0 : ohms;  }

    int getJ2Pin() const {  return j2Pin;  }   // index dans Esp32Pins::ENTREES_J2, ou -1 si non connecte
    void setJ2Pin(int index) {  j2Pin = index;  }

    // Tension de sortie du pont (V) pour une tension d'entree (V)
    double evalue(double vEntree) const {
        if (r1 + r2 <= 0.0) return 0.0;
        return vEntree * r2 / (r1 + r2);
    }

    // Derniere valeur envoyee a QEMU en millivolts (-1 : rien envoye encore) ;
    // sert a n'envoyer que les changements (mis a jour par Modele)
    int mvEnvoye;

private :
    double r1;
    double r2;
    int j2Pin;
};

#endif // PONTDIV_H
