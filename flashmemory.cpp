#include "hex_file.h"
#include "flashmemory.h"
#include "instructions.h"
#include "processeur.h"

Segment::Segment(DWord start, DWord size) {
    debut = start;
    fin = start + size;
    data.assign(size, 0);
}

Segment::~Segment() {
    data.clear();
    debut = 0;
    fin = 0;
}

int Segment::write(DWord adr, Byte d) {
    if (adr<debut) return -1;
    if (adr>=fin) return -1;
    adr -= debut;
    data[adr] = d;
    return 0;
}

int Segment::read(DWord adr, Byte &d) {
    if ((adr<debut)||(adr>=fin)) {
        d=0;
        return -1;
    }
    adr -= debut;
    d = data[adr];
    return 0;
}

int Segment::readWord(DWord adr, Word &w) {
    Byte h, l;
    if (read(adr, l)<0) {
        w=0;
        return -1;
    }
    if (read(adr+1, h)<0) {
        w=0;
        return -1;
    }
    w = h*256 + l;
    return 0;
}

SegProgram::SegProgram(DWord size, JeuInstructionsPtr j) : Segment(0, size)
{
    jeu = j;
    instr.assign(size, 0);
    for (unsigned int i=0; i<instr.size(); i++) instr[i]=jeu->nop()->copie();
}

SegProgram::~SegProgram() {
    for (unsigned int i=0; i<instr.size(); i++) delete instr[i];
    instr.clear();
}

int SegProgram::write(DWord adr, Byte d)
{
    if (Segment::write(adr, d)==0) {
        calculInstr(adr-1);
        calculInstr(adr);
        return 0;
    }
    return -1;
}

void SegProgram::calculInstr(DWord adr)
{
    if (adr>=finSeg()) return;
    Word i;
    delete instr[adr];
    if (readWord(adr, i)!=0) {
        instr[adr] = jeu->nop()->copie();
    } else {
        instr[adr] = jeu->decode(i)->copie();
        instr[adr]->preParametres();
    }
}

FlashMemory::FlashMemory(JeuInstructionsPtr j) {
    jeu = j;
    program = new SegProgram(0x8000, jeu);
    config = new SegConfig();
    devID = new SegDevID(0x00, 0x12);
}

FlashMemory::~FlashMemory() {
    delete program;
    delete config;
    delete devID;
}

int FlashMemory::write(DWord adr, Word w) {
    if (writeByte(adr, w&0xFF)<0) return -1;
    if (writeByte(adr+1, w/256)) return -1;
    return 0;
}

int FlashMemory::writeByte(DWord adr, Byte b) {
    if (program->write(adr, b)==0) return 0;
    if (config->write(adr, b)==0) return 0;
    if (devID->write(adr, b)==0) return 0;
    return -1;
}

int FlashMemory::readByte(DWord adr, Byte &b) {
    if (program->read(adr, b)==0) return 0;
    if (config->read(adr, b)==0) return 0;
    if (devID->read(adr, b)==0) return 0;
    b = 0;
    return -1;
}

int FlashMemory::readFromHex(char *filename) {
    ifstream ifs( filename, ifstream::in);
    char s[256];
    LigneDecode ld;
    Word adrH = 0, data;
    DWord adr;

    while (ifs.good()) {
        ifs.getline(s,256);
        if (ifs.good()) {
//            cout << s << endl;
            if (decode(s, &ld) < 0) {
//                cout << "ERREUR" << endl;
            } else {
                if (ld.t == 0) {
                    adr = adrH*0x10000 + ld.a;
                    if (ld.b == 1) {
                        writeByte(adr, ld.data[0]);
 //                       cout << " " << hex << adr << " Octet " << ld.data[0] << endl;
                    } else {
                        for (int i=0; i<ld.b; i+=2,adr+=2) {
                            data = ld.data[i+1]*0x100 + ld.data[i];
                            write(adr, data);
//                            cout << " " << hex << adr << " Mot " << data << endl;
                        }
                    }
                } else if (ld.t == 4) {
                    adrH = ld.data[0]*256+ld.data[1];
//                    cout << "Offset " << hex << adrH << endl;
                }
            }
        }
    }
    ifs.close();
    return 0;
}

SegConfig::SegConfig() : Segment(0x300000, 0xE)
{
    Segment::write(0x300000, 0x00);
    Segment::write(0x300001, 0x05);
    Segment::write(0x300002, 0x1F);
    Segment::write(0x300003, 0x1F);
    Segment::write(0x300005, 0x83);
    Segment::write(0x300006, 0x85);
    Segment::write(0x300008, 0x0F);
    Segment::write(0x300009, 0xC0);
    Segment::write(0x30000A, 0x0F);
    Segment::write(0x30000B, 0xE0);
    Segment::write(0x30000C, 0x0F);
    Segment::write(0x30000D, 0x40);
}

int SegConfig::write(DWord adr, Byte d)
{
    Byte m = 0;
    switch (adr) {
        case 0x300000 : m = 0x3F;  break;
        case 0x300001 : m = 0xCF;  break;
        case 0x300002 : m = 0x3F;  break;
        case 0x300003 : m = 0x1F;  break;
        case 0x300005 : m = 0x87;  break;
        case 0x300006 : m = 0xE5;  break;
        case 0x300008 : m = 0x0F;  break;
        case 0x300009 : m = 0xC0;  break;
        case 0x30000A : m = 0x0F;  break;
        case 0x30000B : m = 0xE0;  break;
        case 0x30000C : m = 0x0F;  break;
        case 0x30000D : m = 0x40;  break;
    }
    return Segment::write(adr, d & m);
}
