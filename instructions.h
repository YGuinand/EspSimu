#ifndef _INSTRUCTIONS_H
#define _INSTRUCTIONS_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

#include "general.h"

class Instruction;
typedef Instruction *InstructionPtr;

class P18f4550;
// #include "processeur.h"

using namespace std;

class Instruction {
private :
    Word mask, opcode;
    string name;
protected :
    Word instr;
    int code;
    P18f4550 *processeur;
public :
    enum {NOP=0,
          ADDWF,
          ADDWFC,
          ANDWF,
          CLRF,
          COMF,
          CPFSEQ,
          CPFSGT,
          CPFSLT,
          DECF,
          DECFSZ,
          DCFSNZ,
          INCF,
          INCFSZ,
          INFSNZ,
          IORWF,
          MOVF,
          MOVFF,
          MOVWF,
          MULWF,
          NEGF,
          RLCF,
          RLNCF,
          RRCF,
          RRNCF,
          SETF,
          SUBFWB,
          SUBWF,
          SUBWFB,
          SWAPF,
          TSTFSZ,
          XORWF,
          BCF,
          BSF,
          BTFSC,
          BTFSS,
          BTG,
          BC,
          BN,
          BNC,
          BNN,
          BNOV,
          BNZ,
          BOV,
          BRA,
          BZ,
          CALL,
          CLRWDT,
          DAW,
          GOTO,
          POP,
          PUSH,
          RCALL,
          RESET,
          RETFIE,
          RETLW,
          RETURN,
          SLEEP,
          ADDLW,
          ANDLW,
          IORLW,
          LFSR,
          MOVLB,
          MOVLW,
          MULLW,
          SUBLW,
          XORLW,
          TBLRD,
          TBLRD_P,
          TBLRD_M,
          TBLRD_E,
          TBLWT,
          TBLWT_P,
          TBLWT_M,
          TBLWT_E
         };

    Instruction(string n, Word m, Word o, int c, P18f4550 *p);
    virtual ~Instruction() {}
    bool decode(Word i);                // q1
    virtual void preParametres() {}     // normalement en q2 mais ne changent pas et donc precalcule
    virtual void parametres() {}        // q2
    virtual void process() {}           // q3
    virtual bool execute();             // q4
    virtual void specialQ2() {}          // q2 pour 2ème mot
    virtual void specialQ4() {}          // q4 pour 2ème mot
    string mnemo() const {  return name;  }
    int numero() const {  return code;  }
    virtual string affiche() const { return mnemo(); }
    Word getInstr() {  return instr;  }
    virtual InstructionPtr copie() {  return new Instruction(*this); }
};

class InstructionLiteral : public Instruction {
    private :
    Byte k;
    public :
    InstructionLiteral(string n, Word m, Word o, int c, P18f4550 *p) : Instruction(n, m, o, c, p) {}
    virtual void preParametres() { k = instr & 0x00FF; }
    virtual void process();          // q3
    virtual bool execute();          // q4
    virtual string affiche() const { stringstream oss;  oss << mnemo() << " 0x" << hex << (int)k;  return oss.str(); }
    virtual InstructionPtr copie() {  return new InstructionLiteral(*this); }
};

class InstructionByteOriented : public Instruction {
    private :
    bool d, a;
    Byte f8, fReg;
    bool condition;
    public :
    InstructionByteOriented(string n, Word m, Word o, int c, P18f4550 *p) : Instruction(n, m, o, c, p) {}
    virtual void preParametres();
    virtual void parametres();
    virtual void process();          // q3
    virtual bool execute();          // q4
    virtual string affiche() const;
    virtual InstructionPtr copie() {  return new InstructionByteOriented(*this); }
};

class InstructionBitOriented : public Instruction {
    private :
    bool a;
    Byte b, f8, fReg;
    bool condition;
    public :
    InstructionBitOriented(string n, Word m, Word o, int c, P18f4550 *p) : Instruction(n, m, o, c, p) {}
    virtual void preParametres();
    virtual void parametres();
    virtual void process();          // q3
    virtual bool execute();          // q4
    virtual string affiche() const;
    virtual InstructionPtr copie() {  return new InstructionBitOriented(*this); }
};

class InstructionControl8bits : public Instruction {
    protected :
    DWord n, newPC;
    public :
    InstructionControl8bits(string n, Word m, Word o, int c, P18f4550 *p) : Instruction(n, m, o, c, p) {}
    virtual void preParametres() { n = instr & 0x00FF;  if (n & 0x80) n += 0xFFFFFF00; }
    virtual void process();          // q3
    virtual bool execute();          // q4
    virtual string affiche() const { stringstream oss;  oss << mnemo() << " " << (int)n;  return oss.str(); }
    virtual InstructionPtr copie() {  return new InstructionControl8bits(*this); }
};

class InstructionControl11bits : public InstructionControl8bits {
    public :
    InstructionControl11bits(string n, Word m, Word o, int c, P18f4550 *p) : InstructionControl8bits(n, m, o, c, p) {}
    virtual void preParametres() { n = instr & 0x07FF;  if (n & 0x0400) n += 0xFFFFF800; }
    virtual bool execute();          // q4
    virtual InstructionPtr copie() {  return new InstructionControl11bits(*this); }
};

class InstructionControl20bits : public InstructionControl8bits {
    private :
    bool s;
    public :
    InstructionControl20bits(string n, Word m, Word o, int c, P18f4550 *p) : InstructionControl8bits(n, m, o, c, p) {}
    virtual void preParametres();
    virtual void parametres();
    virtual void process();          // q3
    virtual bool execute();          // q4
    virtual string affiche() const { stringstream oss;  oss << mnemo() << " 0x" << hex << (int)n << ", " << (int)s;  return oss.str(); }
    virtual InstructionPtr copie() {  return new InstructionControl20bits(*this); }
};

class InstructionReturn : public Instruction {
    private :
    bool s;
    public :
    InstructionReturn(P18f4550 *p) : Instruction("RETURN"   , 0xFFFE, 0x0012, RETURN, p) {}
    virtual void preParametres() { s = instr & 0x0001; }
    virtual void process();          // q3
    virtual bool execute();          // q4
    virtual string affiche() const { stringstream oss;  oss << mnemo() << " " << (int)s;  return oss.str(); }
    virtual InstructionPtr copie() {  return new InstructionReturn(*this); }
};

class InstructionMovff : public Instruction {
    private :
    Byte fReg;
    Word fdest, fsrc;
    public :
    InstructionMovff(P18f4550 *p) : Instruction("MOVFF"    , 0xF000, 0xC000, MOVFF, p) {}
    virtual void preParametres();
    virtual void parametres();
    virtual bool execute() {  return false;  }          // q4
    virtual void specialQ4();    // q4 pour 2ème mot
    virtual string affiche() const { stringstream oss;  oss << mnemo() << " 0x" << hex << (int)fsrc << ", 0x" << (int)fdest;  return oss.str(); }
    virtual InstructionPtr copie() {  return new InstructionMovff(*this); }
};

class InstructionReset : public Instruction {
    public :
    InstructionReset(P18f4550 *p) : Instruction("RESET"    , 0xFFFF, 0x00FF, RESET, p) {}
    virtual void parametres();
    virtual InstructionPtr copie() {  return new InstructionReset(*this); }
};

class InstructionPush : public Instruction {
    public :
    InstructionPush(P18f4550 *p) : Instruction("PUSH"     , 0xFFFF, 0x0005, PUSH, p) {}
    virtual void parametres();
    virtual InstructionPtr copie() {  return new InstructionPush(*this); }
};

class InstructionRCall : public InstructionControl11bits {
    public :
    InstructionRCall(P18f4550 *p) : InstructionControl11bits("RCALL"    , 0xF800, 0xD800, RCALL, p) {}
    virtual void parametres();
    virtual InstructionPtr copie() {  return new InstructionRCall(*this); }
};

class InstructionPop : public Instruction {
    public :
    InstructionPop(P18f4550 *p) : Instruction("POP"      , 0xFFFF, 0x0006, POP, p) {}
    virtual void process();          // q3
    virtual InstructionPtr copie() {  return new InstructionPop(*this); }
};

class InstructionLfsr : public Instruction {
    private :
        Byte kH, kL, f;
    public :
    InstructionLfsr(P18f4550 *p) : Instruction("LFSR"     , 0xFFC0, 0xEE00, LFSR, p) {}
    virtual void preParametres();
    virtual void parametres();       // q2
    virtual bool execute();          // q4
    virtual void specialQ4();          // q4 pour 2ème mot
    virtual string affiche() const { int k=kH;  k=(k<<8)+kL;  stringstream oss;  oss << mnemo() << " " << (int)f << ", 0x" << hex << k;  return oss.str(); }
    virtual InstructionPtr copie() {  return new InstructionLfsr(*this); }
};

class InstructionDaw : public Instruction {
    public :
    InstructionDaw(P18f4550 *p) : Instruction("DAW"      , 0xFFFF, 0x0007, DAW, p) {}
    virtual bool execute();          // q4
    virtual InstructionPtr copie() {  return new InstructionDaw(*this); }
};

class InstructionTblrd : public Instruction {
    public :
    InstructionTblrd(string n, Word m, Word o, int c, P18f4550 *p) : Instruction(n, m, o, c, p) {}
    virtual bool execute();          // q4
    virtual void specialQ4();          // q4 pour 2ème mot
    virtual InstructionPtr copie() {  return new InstructionTblrd(*this); }
};

#endif
