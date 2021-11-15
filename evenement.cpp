#include "evenement.h"

Evenement::Evenement()
{
    enregistrement = false;
    precedent = NULL;
    suivant = NULL;
}

void Evenement::supprimer()
{
//    if (!enregistrement) {
//        return;
//    }
    if (suivant) suivant->attacherPrecedent(precedent);
    if (precedent) precedent->attacherSuivant(suivant);
    else liste->attacher(numeroEnregistrement(), suivant);
    enregistrement = false;
    precedent = NULL;
    suivant = NULL;
}

void Evenement::enregistrer(uint16_t temps, ListeEvts *li)
{
    liste = li;
    liste->enregistrer(temps, this);
    enregistrement = true;
    instantEnreg = liste->maintenant();
}

void Evenement::enregistrerDernier(uint16_t temps, ListeEvts *li)
{
    liste = li;
    liste->enregistrerDernier(temps, this);
    enregistrement = true;
    instantEnreg = liste->maintenant();
}

ListeEvts::ListeEvts()
{
    for (int i=0; i<65536; i++) table[i] = NULL;
    now = 0;
}

ListeEvts::~ListeEvts()
{
    for (int i=0; i<65536; i++) {
        while (table[i]) table[i]->supprimer();
    }
}

void ListeEvts::attacher(uint16_t num, EvenementPtr evt)
{
    table[num] = evt;
}

void ListeEvts::enregistrer(uint16_t temps, EvenementPtr evt)
{
    uint16_t num = now + temps;

    if (table[num]) {
        table[num]->attacherPrecedent(evt);
    }
    evt->attacherSuivant(table[num]);
    table[num] = evt;
    evt->setNumeroEnregistrement(num);
}

void ListeEvts::enregistrerDernier(uint16_t temps, EvenementPtr evt)
{
    uint16_t num = now + temps;
    EvenementPtr src;

    if (table[num]) {
        src = table[num];
        while (src->getSuivant()) src = src->getSuivant();
        src->attacherSuivant(evt);
        evt->attacherPrecedent(src);
    } else {
        table[num] = evt;
    }
    evt->setNumeroEnregistrement(num);
}

void ListeEvts::executer()
{
    while (table[now]) {
        table[now]->executer();
    }
    now++;
}
