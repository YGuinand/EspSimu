#ifndef _GENERAL_H
#define _GENERAL_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <stdint.h>

#include <QDebug>
extern QDebug *dbg;

using namespace std;

typedef uint16_t Word;
typedef uint32_t DWord;
typedef uint8_t Byte;

#define MASK(n) (1 << n)
#define MASK0   0x01
#define MASK1   0x02
#define MASK2   0x04
#define MASK3   0x08
#define MASK4   0x10
#define MASK5   0x20
#define MASK6   0x40
#define MASK7   0x80

#endif
