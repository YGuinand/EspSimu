#ifndef EVENEMENT_H
#define EVENEMENT_H

#include "general.h"

class Evenement;
typedef Evenement *EvenementPtr;

class ListeEvts;
typedef ListeEvts *ListeEvtsPtr;

class Evenement
{
private :
    bool enregistrement;
    uint16_t numEnreg, instantEnreg;
    EvenementPtr precedent, suivant;
    ListeEvtsPtr liste;
public :
    Evenement();
    void supprimer();
    void enregistrer(uint16_t temps, ListeEvtsPtr li);
    void enregistrerDernier(uint16_t temps, ListeEvtsPtr li);
    bool estEnregistre() {  return enregistrement;  }
    uint16_t numeroEnregistrement() {  return numEnreg;  }
    void setNumeroEnregistrement(uint16_t num) { numEnreg = num; }
    virtual void executer() {}
    void attacherSuivant(EvenementPtr s) {  suivant = s;  }
    void attacherPrecedent(EvenementPtr p) {  precedent = p;  }
    uint16_t tempsEcoule();
    virtual void reset() {  if (estEnregistre()) supprimer();  }
    EvenementPtr getSuivant() {  return suivant;  }
    EvenementPtr getPrecedent() {  return precedent;  }
};

class ListeEvts
{
private :
    EvenementPtr table[65536];
    uint16_t now;
public :
    ListeEvts();
    ~ListeEvts();
    void enregistrer(uint16_t temps, EvenementPtr evt);
    void enregistrerDernier(uint16_t temps, EvenementPtr evt);
    void attacher(uint16_t num, EvenementPtr evt);
    void executer();
    uint16_t maintenant() {  return now;  }
};

inline uint16_t Evenement::tempsEcoule() {  return liste->maintenant() - instantEnreg;  }

#endif // EVENEMENT_H
