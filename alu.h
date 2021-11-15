#ifndef ALU_H
#define ALU_H

#include "general.h"

class P18f4550;

class Alu {
public :
    enum { MASK_NONE = 0x00,
           MASK_C = 0x01,
           MASK_DC = 0x02,
           MASK_Z = 0x04,
           MASK_OV = 0x08,
           MASK_N = 0x10,
           MASK_ZN = 0x14,
           MASK_CZN = 0x15,
           MASK_ALL = 0x1F };
private :
    P18f4550 *processeur;
    Byte alu;
    Byte status, mask;
    Byte carry() {  return status & MASK_C;  }
    void setCarry(bool c=true) {  if (c) status |= MASK_C; else status &= ~MASK_C;  }
    Byte digitCarry() {  return (status & MASK_DC)!=0;  }
    void setDigitCarry(bool c=true) {  if (c) status |= MASK_DC; else status &= ~MASK_DC;  }
    Byte zero() {  return (status & MASK_Z)!=0;  }
    void setZero(bool c=true) {  if (c) status |= MASK_Z; else status &= ~MASK_Z;  }
    Byte overflow() {  return (status & MASK_OV)!=0;  }
    void setOverflow(bool c=true) {  if (c) status |= MASK_OV; else status &= ~MASK_OV;  }
    Byte negative() {  return (status & MASK_N)!=0;  }
    void setNegative(bool c=true) {  if (c) status |= MASK_N; else status &= ~MASK_N;  }

public :
    Alu(P18f4550 *p);
    void reset();
    Byte read() {  return alu;  }
    void writeStatus();
    bool isZero() { return alu == 0; }

    void operationMOV(Byte m, Byte a);
    void operationMOVW(Byte m);
    void operationADD(Byte m, Byte a, bool c = false);
    void operationSUB(Byte m, Byte a, bool c = false);
    void operationSUBW(Byte m, Byte a, bool c = false);
    void operationDEC(Byte m, Byte a);
    void operationINC(Byte m, Byte a);
    void operationNEG(Byte m, Byte a);
    void operationAND(Byte m, Byte a);
    void operationOR(Byte m, Byte a);
    void operationXOR(Byte m, Byte a);
    void operationCOM(Byte m, Byte a);
    void operationCLR(Byte m);
    void operationSET(Byte m);
    void operationLEFT(Byte m, Byte a, bool c = false);
    void operationRIGHT(Byte m, Byte a, bool c = false);
    void operationSWAP(Byte m, Byte a);
};

#endif // ALU_H
