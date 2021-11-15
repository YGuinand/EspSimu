#ifndef _FLASHMEMORY_H
#define _FLASHMEMORY_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

#include "general.h"

using namespace std;

class Instruction;
typedef Instruction *InstructionPtr;

class JeuInstructions;
typedef JeuInstructions *JeuInstructionsPtr;

class Segment {
private :
    DWord debut, fin;
    vector<Byte> data;
public :
    Segment(DWord start, DWord size);
    virtual ~Segment();
    virtual int write(DWord adr, Byte d);
    virtual int read(DWord adr, Byte &d);
    DWord finSeg() {  return fin;  }
    DWord debutSeg() {  return debut;  }
    int readWord(DWord adr, Word &w);
};

class SegProgram : public Segment {
private :
    vector<InstructionPtr> instr;
    JeuInstructionsPtr jeu;
public :
    SegProgram(DWord size, JeuInstructionsPtr j);
    virtual ~SegProgram();
    virtual int write(DWord adr, Byte d);
    InstructionPtr getInstr(DWord adr) {  return instr[adr];  } // pas de controle de validite de l'adresse
    void calculInstr(DWord adr);
};

class SegConfig : public Segment {
public :
    SegConfig();
    virtual int write(DWord adr, Byte d);
};

class SegDevID : public Segment {
public :
    SegDevID(Byte devID1, Byte devID2) : Segment(0x3FFFFE, 2) {  Segment::write(0x3FFFFE, devID1);  Segment::write(0x3FFFFF, devID2); }
    virtual int write(DWord /* adr */, Byte /* d */) {  return -1;  }
};

typedef Segment *SegmentPtr;
typedef SegProgram *SegProgramPtr;

class FlashMemory {
private :
    SegProgramPtr program;
    SegmentPtr config, devID;
    JeuInstructionsPtr jeu;
public :
    FlashMemory(JeuInstructionsPtr j);
    ~FlashMemory();
    int write(DWord adr, Word w);
    int writeByte(DWord adr, Byte b);
    int readByte(DWord adr, Byte &b);
    int readFromHex(char *filename);

    InstructionPtr getInstr(DWord adr) { return program->getInstr(adr); }

    bool bitPBADEN() { Byte b;  readByte(0x300005, b);  return (b & 0x02)!=0; }
};

#endif
