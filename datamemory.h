#ifndef _DATAMEMORY_H
#define _DATAMEMORY_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

#include "general.h"
#include "processeur.h"
#include "ports.h"
#include "adc.h"
#include "pwm.h"
#include "registre.h"

class DataMemory {
public :
    enum {
        SPPDATA = 0xF62,
        SPPCFG = 0xF63,
        SPPEPS = 0xF64,
        SPPCON = 0xF65,
        UFRML = 0xF66,
        UFRMH = 0xF67,
        UIR = 0xF68,
        UIE = 0xF69,
        UEIR = 0xF6A,
        UEIE = 0xF6B,
        USTAT = 0xF6C,
        UCON = 0xF6D,
        UADDR = 0xF6E,
        UCFG = 0xF6F,
        UEP0 = 0xF70,
        UEP1 = 0xF71,
        UEP2 = 0xF72,
        UEP3 = 0xF73,
        UEP4 = 0xF74,
        UEP5 = 0xF75,
        UEP6 = 0xF76,
        UEP7 = 0xF77,
        UEP8 = 0xF78,
        UEP9 = 0xF79,
        UEP10 = 0xF7A,
        UEP11 = 0xF7B,
        UEP12 = 0xF7C,
        UEP13 = 0xF7D,
        UEP14 = 0xF7E,
        UEP15 = 0xF7F,
        PORTA = 0xF80,
        PORTB = 0xF81,
        PORTC = 0xF82,
        PORTD = 0xF83,
        PORTE = 0xF84,
        LATA = 0xF89,
        LATB = 0xF8A,
        LATC = 0xF8B,
        LATD = 0xF8C,
        LATE = 0xF8D,
        TRISA = 0xF92,
        TRISB = 0xF93,
        TRISC = 0xF94,
        TRISD = 0xF95,
        TRISE = 0xF96,
        OSCTUNE = 0xF9B,
        PIE1 = 0xF9D,
        PIR1 = 0xF9E,
        IPR1 = 0xF9F,
        PIE2 = 0xFA0,
        PIR2 = 0xFA1,
        IPR2 = 0xFA2,
        EECON1 = 0xFA6,
        EECON2 = 0xFA7,
        EEDATA = 0xFA8,
        EEADR = 0xFA9,
        RCSTA = 0xFAB,
        TXSTA = 0xFAC,
        TXREG = 0xFAD,
        RCREG = 0xFAE,
        SPBRG = 0xFAF,
        SPBRGH = 0xFB0,
        T3CON = 0xFB1,
        TMR3L = 0xFB2,
        TMR3H = 0xFB3,
        CMCON = 0xFB4,
        CVRCON = 0xFB5,
        ECCP1AS = 0xFB6,
        ECCP1DEL = 0xFB7,
        BAUDCON = 0xFB8,
        CCP2CON = 0xFBA,
        CCPR2L = 0xFBB,
        CCPR2H = 0xFBC,
        CCP1CON = 0xFBD,
        CCPR1L = 0xFBE,
        CCPR1H = 0xFBF,
        ADCON2 = 0xFC0,
        ADCON1 = 0xFC1,
        ADCON0 = 0xFC2,
        ADRESL = 0xFC3,
        ADRESH = 0xFC4,
        SSPCON2 = 0xFC5,
        SSPCON1 = 0xFC6,
        SSPSTAT = 0xFC7,
        SSPADD = 0xFC8,
        SSPBUF = 0xFC9,
        T2CON = 0xFCA,
        PR2 = 0xFCB,
        TMR2 = 0xFCC,
        T1CON = 0xFCD,
        TMR1L = 0xFCE,
        TMR1H = 0xFCF,
        RCON = 0xFD0,
        WDTCON = 0xFD1,
        HLVDCON = 0xFD2,
        OSCCON = 0xFD3,
        T0CON = 0xFD5,
        TMR0L = 0xFD6,
        TMR0H = 0xFD7,
        STATUS = 0xFD8,
        FSR2L = 0xFD9,
        FSR2H = 0xFDA,
        PLUSW2 = 0xFDB,
        PREINC2 = 0xFDC,
        POSTDEC2 = 0xFDD,
        POSTINC2 = 0xFDE,
        INDF2 = 0xFDF,
        BSR = 0xFE0,
        FSR1L = 0xFE1,
        FSR1H = 0xFE2,
        PLUSW1 = 0xFE3,
        PREINC1 = 0xFE4,
        POSTDEC1 = 0xFE5,
        POSTINC1 = 0xFE6,
        INDF1 = 0xFE7,
        WREG = 0xFE8,
        FSR0L = 0xFE9,
        FSR0H = 0xFEA,
        PLUSW0 = 0xFEB,
        PREINC0 = 0xFEC,
        POSTDEC0 = 0xFED,
        POSTINC0 = 0xFEE,
        INDF0 = 0xFEF,
        INTCON3 = 0xFF0,
        INTCON2 = 0xFF1,
        INTCON = 0xFF2,
        PRODL = 0xFF3,
        PRODH = 0xFF4,
        TABLAT = 0xFF5,
        TBLPTRL = 0xFF6,
        TBLPTRH = 0xFF7,
        TBLPTRU = 0xFF8,
        PCL = 0xFF9,
        PCLATH = 0xFFA,
        PCLATU = 0xFFB,
        STKPTR = 0xFFC,
        TOSL = 0xFFD,
        TOSH = 0xFFE,
        TOSU = 0xFFF
    };
private :
    static const string nameSFR[160];
    RegistrePtr reg[0x1000];
    RegistreStatus *status;
    RegistreBSR *bsr;

    DWord stack[32];
    DWord programCounter;
    Byte bsrS, statusS, wS;
    P18f4550 *processeur;
//    Byte readSFR(Word adr);
    Byte readAccessBank(Byte adr)
        {  if (adr<0x60) return read(adr);   return read(adr+0xF00);  }
    void writeAccessBank(Byte adr, Byte b)
        {  if (adr<0x60) write(adr, b); else write(adr+0xF00, b); }
    void setBitAccessBank(Byte adr, bool b, Byte num)
        {  if (adr<0x60) bitSetClr(adr, b, num); else bitSetClr(adr+0xF00, b, num);  }
public :
    DataMemory(P18f4550 *p);
    ~DataMemory();
    void powerOnReset();
    Byte read(Word adr) {  if (adr>=0x1000) return 0;   return reg[adr]->read();  }
    Byte read(bool a, Byte adrL);
    void write(bool a, Byte adrL, Byte b);
    void bitSetClr(bool a, Byte adrL, bool b, Byte num);
    void write(Word adr, Byte b) {  if (adr>=0x1000) return;   reg[adr]->write(b);  }
    void bitSetClr(Word adr, bool b, Byte num) {  if (adr>=0x1000) return;  reg[adr]->bitSetClr(num, b);  }
    void setBit(Word adr, Byte num) {  if (adr>=0x1000) return;  reg[adr]->setBit(num);  }
    void clrBit(Word adr, Byte num) {  if (adr>=0x1000) return;  reg[adr]->clrBit(num);  }
    void modifie(int i, RegistrePtr p) {  delete reg[i];  reg[i]=p;  }

// BSR register
    Byte readBSR() {  return bsr->readDirect();  }
    void writeBSR(Byte k) {  bsr->write(k);  }

// Program Counter
    void resetPC() {  programCounter = 0;  }
    DWord getPC() {  return programCounter;  }
    DWord addPC(int n) {  return (programCounter + n) & 0x7FFF;  }
    void nextPC() {  programCounter = (programCounter + 2) & 0x7FFF;  }
    void prevPC() {  programCounter = (programCounter - 2) & 0x7FFF;  }
    void setPC(DWord pc) {  programCounter = pc & 0x7FFF;  }

    bool push(DWord pc, bool stvren);
    DWord pop();
    void pushReg() {  wS = w(); statusS = status->readDirect(); bsrS = readBSR();  }
    void popReg(void);

    void produit(Word prod);

// W register
    Byte w() {  return reg[WREG]->readDirect();  }
    void setW(Byte w) {  reg[WREG]->writeDirect(w);  }

    DWord readTBLPTR();
    void incTBLPTR();
    void decTBLPTR();
    void writeTABLAT(Byte b) {  reg[TABLAT]->writeDirect(b);  }

// STATUS register
    Byte readStatus() {  return status->readDirect();  }
    void writeStatus(Byte b, Byte m = 0x1F) {  status->writeMask(b, m); }
    Byte carry() {  return status->carry();  }
    Byte digitCarry() {  return status->digitCarry();  }
    void setCarry(bool c=true) {  status->setCarry(c);  }
    void setDigitCarry(bool c=true) {  status->setDigitCarry(c);  }
    Byte negative() {  return status->negative();  }
    void setNegative(bool c=true) {  status->setNegative(c);  }
    Byte overflow() {  return status->overflow();  }
    void setOverflow(bool c=true) {  status->setOverflow(c);  }
    Byte zero() {  return status->zero();  }
    void setZero(bool c=true) {  status->setZero(c);  }

    Word getAdr(bool a, Byte adrL);
    string name(Word adr);
    string name(bool a, Byte adrL) {  return name(getAdr(a, adrL)); }

// Adressage indirect
    void writeFSRfH(Byte f, Byte b);
    void writeFSRfL(Byte f, Byte b);
    void doPreIndirect(Word adr) {  reg[adr]->doPreIndirect();  }
    void doPostIndirect(Word adr) {  reg[adr]->doPostIndirect();  }
    void doPreIndirect(bool a, Byte adrL) { doPreIndirect(getAdr(a, adrL)); }
    void doPostIndirect(bool a, Byte adrL) { doPostIndirect(getAdr(a, adrL)); }
    Word readFSR0() {  return (((Word)reg[FSR0H]->readDirect())<<8) + reg[FSR0L]->readDirect();  }
    Word readFSR1() {  return (((Word)reg[FSR1H]->readDirect())<<8) + reg[FSR1L]->readDirect();  }
    Word readFSR2() {  return (((Word)reg[FSR2H]->readDirect())<<8) + reg[FSR2L]->readDirect();  }
    void incFSR0() {  reg[FSR0L]->incDirect(); if (reg[FSR0L]->readDirect()==0) reg[FSR0H]->writeDirect((reg[FSR0H]->readDirect()+1)&0x0F);  }
    void incFSR1() {  reg[FSR1L]->incDirect(); if (reg[FSR1L]->readDirect()==0) reg[FSR1H]->writeDirect((reg[FSR1H]->readDirect()+1)&0x0F);  }
    void incFSR2() {  reg[FSR2L]->incDirect(); if (reg[FSR2L]->readDirect()==0) reg[FSR2H]->writeDirect((reg[FSR2H]->readDirect()+1)&0x0F);  }
    void decFSR0() {  reg[FSR0L]->decDirect(); if (reg[FSR0L]->readDirect()==0xFF) reg[FSR0H]->writeDirect((reg[FSR0H]->readDirect()-1)&0x0F);  }
    void decFSR1() {  reg[FSR1L]->decDirect(); if (reg[FSR1L]->readDirect()==0xFF) reg[FSR1H]->writeDirect((reg[FSR1H]->readDirect()-1)&0x0F);  }
    void decFSR2() {  reg[FSR2L]->decDirect(); if (reg[FSR2L]->readDirect()==0xFF) reg[FSR2H]->writeDirect((reg[FSR2H]->readDirect()-1)&0x0F);  }
    bool isIndirectReg(Word adr) {  return reg[adr]->isIndirectReg();  }

// PORTx register
    void writePortADirect(Byte b) {  reg[PORTA]->writeDirect(b);  }
    void writePortBDirect(Byte b) {  reg[PORTB]->writeDirect(b);  }
    void writePortCDirect(Byte b) {  reg[PORTC]->writeDirect(b);  }
    void writePortDDirect(Byte b) {  reg[PORTD]->writeDirect(b);  }
    void writePortEDirect(Byte b) {  reg[PORTE]->writeDirect(b);  }

};

#endif
