#include "registre.h"
#include "datamemory.h"

const Byte Registre::maskSet[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
const Byte Registre::maskClr[8] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F};

#ifdef Q_OS_WASM
#define inline
#endif

inline void RegistrePREINC0::doPreIndirect() {  mem->incFSR0();  }
inline void RegistrePREINC1::doPreIndirect() {  mem->incFSR1();  }
inline void RegistrePREINC2::doPreIndirect() {  mem->incFSR2();  }
inline void RegistrePOSTINC0::doPostIndirect() {  mem->incFSR0();  }
inline void RegistrePOSTINC1::doPostIndirect() {  mem->incFSR1();  }
inline void RegistrePOSTINC2::doPostIndirect() {  mem->incFSR2();  }
inline void RegistrePOSTDEC0::doPostIndirect() {  mem->decFSR0();  }
inline void RegistrePOSTDEC1::doPostIndirect() {  mem->decFSR1();  }
inline void RegistrePOSTDEC2::doPostIndirect() {  mem->decFSR2();  }
inline void RegistreIndirect0::write(Byte d) {  int adr = mem->readFSR0(); if (!mem->isIndirectReg(adr)) mem->write(adr, d);  }
inline void RegistreIndirect0::setBit(Byte num) {  int adr = mem->readFSR0(); if (!mem->isIndirectReg(adr)) mem->setBit(adr, num);  }
inline void RegistreIndirect0::clrBit(Byte num) {  int adr = mem->readFSR0(); if (!mem->isIndirectReg(adr)) mem->clrBit(adr, num);  }
inline Byte RegistreIndirect0::read() {  int adr = mem->readFSR0(); return (mem->isIndirectReg(adr)) ? 0 : mem->read(adr);  }
inline void RegistreIndirect1::write(Byte d) {  int adr = mem->readFSR1(); if (!mem->isIndirectReg(adr)) mem->write(adr, d);  }
inline void RegistreIndirect1::setBit(Byte num) {  int adr = mem->readFSR1(); if (!mem->isIndirectReg(adr)) mem->setBit(adr, num);  }
inline void RegistreIndirect1::clrBit(Byte num) {  int adr = mem->readFSR1(); if (!mem->isIndirectReg(adr)) mem->clrBit(adr, num);  }
inline Byte RegistreIndirect1::read() {  int adr = mem->readFSR1(); return (mem->isIndirectReg(adr)) ? 0 : mem->read(adr);  }
inline void RegistreIndirect2::write(Byte d) {  int adr = mem->readFSR2(); if (!mem->isIndirectReg(adr)) mem->write(adr, d);  }
inline void RegistreIndirect2::setBit(Byte num) {  int adr = mem->readFSR2(); if (!mem->isIndirectReg(adr)) mem->setBit(adr, num);  }
inline void RegistreIndirect2::clrBit(Byte num) {  int adr = mem->readFSR2(); if (!mem->isIndirectReg(adr)) mem->clrBit(adr, num);  }
inline Byte RegistreIndirect2::read() {  int adr = mem->readFSR2(); return (mem->isIndirectReg(adr)) ? 0 : mem->read(adr);  }
inline Word RegistrePLUSW0::calcAdr() {  Word w = mem->w(); if (w & 0x80) w+=0xFF00; return (mem->readFSR0()+w) & 0x0FFF;  }
inline void RegistrePLUSW0::write(Byte d) {  Word adr = calcAdr(); if (!mem->isIndirectReg(adr)) mem->write(adr, d);  }
inline void RegistrePLUSW0::setBit(Byte num) {  Word adr = calcAdr(); if (!mem->isIndirectReg(adr)) mem->setBit(adr, num);  }
inline void RegistrePLUSW0::clrBit(Byte num) {  Word adr = calcAdr(); if (!mem->isIndirectReg(adr)) mem->clrBit(adr, num);  }
inline Byte RegistrePLUSW0::read() {  Word adr = calcAdr(); return (mem->isIndirectReg(adr)) ? 0 : mem->read(adr);  }
inline Word RegistrePLUSW1::calcAdr() {  Word w = mem->w(); if (w & 0x80) w+=0xFF00; return (mem->readFSR1()+w) & 0x0FFF;  }
inline void RegistrePLUSW1::write(Byte d) {  Word adr = calcAdr(); if (!mem->isIndirectReg(adr)) mem->write(adr, d);  }
inline void RegistrePLUSW1::setBit(Byte num) {  Word adr = calcAdr(); if (!mem->isIndirectReg(adr)) mem->setBit(adr, num);  }
inline void RegistrePLUSW1::clrBit(Byte num) {  Word adr = calcAdr(); if (!mem->isIndirectReg(adr)) mem->clrBit(adr, num);  }
inline Byte RegistrePLUSW1::read() {  Word adr = calcAdr(); return (mem->isIndirectReg(adr)) ? 0 : mem->read(adr);  }
inline Word RegistrePLUSW2::calcAdr() {  Word w = mem->w(); if (w & 0x80) w+=0xFF00; return (mem->readFSR2()+w) & 0x0FFF;  }
inline void RegistrePLUSW2::write(Byte d) {  Word adr = calcAdr(); if (!mem->isIndirectReg(adr)) mem->write(adr, d);  }
inline void RegistrePLUSW2::setBit(Byte num) {  Word adr = calcAdr(); if (!mem->isIndirectReg(adr)) mem->setBit(adr, num);  }
inline void RegistrePLUSW2::clrBit(Byte num) {  Word adr = calcAdr(); if (!mem->isIndirectReg(adr)) mem->clrBit(adr, num);  }
inline Byte RegistrePLUSW2::read() {  Word adr = calcAdr(); return (mem->isIndirectReg(adr)) ? 0 : mem->read(adr);  }
