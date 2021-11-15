#include "alu.h"
#include "processeur.h"

Alu::Alu(P18f4550 *p) : processeur(p) {}

void Alu::reset() {
    status = processeur->getDM()->readStatus();
    alu = 0;
}

void Alu::writeStatus() {
    processeur->getDM()->writeStatus(status, mask);
}

void Alu::operationMOV(Byte m, Byte a)
{
    mask = m;
    status = processeur->getDM()->readStatus();
    alu = a;
    setZero(alu == 0);
    setNegative(alu & 0x80);
}

void Alu::operationMOVW(Byte m)
{
    mask = m;
    status = processeur->getDM()->readStatus();
    alu = processeur->getDM()->w();
}

void Alu::operationADD(Byte m, Byte a, bool c)
{
    mask = m;
    status = processeur->getDM()->readStatus();
    Word res;
    bool n, na, nb;
    Byte b = processeur->getDM()->w();
    res = (Word)a + (Word)b;
    if (c && carry()) res++;
    setCarry(res > 255);
    alu = res;
    setZero(alu == 0);
    n = alu & 0x80;
    setNegative(n);
    res = (a & 0x0F) + (b & 0x0F);
    setDigitCarry(res > 15);
    na = a & 0x80;
    nb = b & 0x80;
    setOverflow((na && nb && !n) || (!na && !nb && n));
}

void Alu::operationSUB(Byte m, Byte a, bool c)
{
    mask = m;
    status = processeur->getDM()->readStatus();
    Word res;
    bool n, na, nb;
    Byte b = (~processeur->getDM()->w());
    res = (Word)a + (Word)b + 1;
    if (c && !carry()) res--;
    setCarry(res > 255);
    alu = res;
    setZero(alu == 0);
    n = alu & 0x80;
    setNegative(n);
    res = (a & 0x0F) + (b & 0x0F);
    setDigitCarry(res > 15);
    na = a & 0x80;
    nb = b & 0x80;
    setOverflow((na && nb && !n) || (!na && !nb && n));
}

void Alu::operationSUBW(Byte m, Byte a, bool c)
{
    mask = m;
    status = processeur->getDM()->readStatus();
    Word res;
    bool n, na, nb;
    Byte b = processeur->getDM()->w();
    a = ~a;
    res = (Word)a + (Word)b + 1;
    if (c && !carry()) res--;
    setCarry(res > 255);
    alu = res;
    setZero(alu == 0);
    n = alu & 0x80;
    setNegative(n);
    res = (a & 0x0F) + (b & 0x0F);
    setDigitCarry(res > 15);
    na = a & 0x80;
    nb = b & 0x80;
    setOverflow((na && nb && !n) || (!na && !nb && n));
}

void Alu::operationDEC(Byte m, Byte a)
{
    mask = m;
    status = processeur->getDM()->readStatus();
    Word res;
    bool n, na;
    res = (Word)a + (Word)0xFF;
    setCarry(res > 255);
    alu = res;
    setZero(alu == 0);
    n = alu & 0x80;
    setNegative(n);
    res = (a & 0x0F) + 0x0F;
    setDigitCarry(res > 15);
    na = a & 0x80;
    setOverflow(na && !n);
}

void Alu::operationINC(Byte m, Byte a)
{
    mask = m;
    status = processeur->getDM()->readStatus();
    Word res;
    bool n, na;
    res = (Word)a + (Word)0x01;
    setCarry(res > 255);
    alu = res;
    setZero(alu == 0);
    n = alu & 0x80;
    setNegative(n);
    res = (a & 0x0F) + 0x01;
    setDigitCarry(res > 15);
    na = a & 0x80;
    setOverflow((!na) && n);
}

void Alu::operationNEG(Byte m, Byte a)
{
    mask = m;
    status = processeur->getDM()->readStatus();
    Word res;
    bool n;
    res = (Word)(~a) + 1;
//    setCarry(a == 0x80);
    setCarry(false);
    alu = res;
    setZero(alu == 0);
    n = alu & 0x80;
    setNegative(n);
    res = (a & 0x0F);
//    setDigitCarry(res == 0x08);
    setDigitCarry(false);
    setOverflow(a == 0x80);
}

void Alu::operationAND(Byte m, Byte a)
{
    mask = m;
    status = processeur->getDM()->readStatus();
    Byte b = processeur->getDM()->w();
    alu = a & b;
    setZero(alu == 0);
    setNegative(alu & 0x80);
}

void Alu::operationOR(Byte m, Byte a)
{
    mask = m;
    status = processeur->getDM()->readStatus();
    Byte b = processeur->getDM()->w();
    alu = a | b;
    setZero(alu == 0);
    setNegative(alu & 0x80);
}

void Alu::operationXOR(Byte m, Byte a)
{
    mask = m;
    status = processeur->getDM()->readStatus();
    Byte b = processeur->getDM()->w();
    alu = a ^ b;
    setZero(alu == 0);
    setNegative(alu & 0x80);
}

void Alu::operationCOM(Byte m, Byte a)
{
    mask = m;
    status = processeur->getDM()->readStatus();
    alu = ~a;
    setZero(alu == 0);
    setNegative(alu & 0x80);
}

void Alu::operationCLR(Byte m)
{
    mask = m;
    status = processeur->getDM()->readStatus();
    alu = 0;
    setZero();
}

void Alu::operationSET(Byte m)
{
    mask = m;
    status = processeur->getDM()->readStatus();
    alu = 0xFF;
}

void Alu::operationLEFT(Byte m, Byte a, bool c)
{
    mask = m;
    status = processeur->getDM()->readStatus();
    alu = a << 1;
    if (c) {
        if (carry()) alu++;
        setCarry(a & 0x80);
    } else {
        if (a & 0x80) alu++;
    }
    setZero(alu == 0);
    setNegative(alu & 0x80);
}

void Alu::operationRIGHT(Byte m, Byte a, bool c)
{
    mask = m;
    status = processeur->getDM()->readStatus();
    alu = a >> 1;
    if (c) {
        if (carry()) alu += 0x80;
        setCarry(a & 0x01);
    } else {
        if (a & 0x01) alu += 0x80;
    }
    setZero(alu == 0);
    setNegative(alu & 0x80);
}

void Alu::operationSWAP(Byte m, Byte a)
{
    mask = m;
    status = processeur->getDM()->readStatus();
    alu = (a >> 4) + (a << 4);
}
