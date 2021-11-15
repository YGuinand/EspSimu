#ifndef _HEX_FILE_H
#define _HEX_FILE_H

typedef struct {
    int a, b, t;
    int data[255];
} LigneDecode;

int decode(char *s, LigneDecode *ld);

#endif
