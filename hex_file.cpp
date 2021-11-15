#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

#include "hex_file.h"

using namespace std;

int hexDigit(char *s) {
    if ((*s>='0') && (*s<='9')) {
        return *s - '0';
    } else if ((*s>='a') && (*s<='f')) {
        return *s - 'a' + 10;
    } else if ((*s>='A') && (*s<='F')) {
        return *s - 'A' + 10;
    }
    return -1; // erreur
}

int hexByte(char *s) {
    int vh, vl;

    vh = hexDigit(s);
    if (vh < 0) return -1; // erreur
    s++;
    vl = hexDigit(s);
    if (vl < 0) return -1; // erreur
    return vh*16+vl;
}

int decode(char *s, LigneDecode *ld) {
    int ah, al, sum, check, i;

    if (*s++ != ':') return -1; // la ligne doit commencer par : sinon on passe à la suivante
    ld->b = hexByte(s);
    if (ld->b < 0) return -2; // la ligne ne contient pas de données
    sum = ld->b;
    s += 2;
    ah = hexByte(s);
    if (ah < 0) return -3; // erreur adresse
    sum += ah;
    s += 2;
    al = hexByte(s);
    if (al < 0) return -4;
    sum += al;
    s += 2;
    ld->a = ah * 256 + al;
    ld->t = hexByte(s);
    if (ld->t < 0) return -5;
    sum += ld->t;
    s += 2;
    if (ld->t == 1) { // fin de fichier
        check = hexByte(s);
        if (check < 0) return -7;
        if (((sum + check) & 0xFF) == 0) {
//            printf("Fin de fichier\n");
            return 0; // ok
        }
        return -8; // checksum erreur
    } else if ((ld->t != 0) && (ld->t != 4)) return -6; // t doit être 0, 1 ou 4
    for (i=0; i<ld->b; i++) {
        ld->data[i] = hexByte(s);
        if (ld->data[i] < 0) return -9;
        sum += ld->data[i];
        s += 2;
    }
    check = hexByte(s);
    if (check < 0) return -10;
    if (((sum + check) & 0xFF) == 0) {
//        printf("%X %04X", ld->t, ld->a);
//        for (i=0; i<ld->b; i++) printf(" %02X", ld->data[i]);
//        printf("\n");
        return 0; // ok
    }
    return -11; // checksum erreur
}

