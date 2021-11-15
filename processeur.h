#ifndef _PROCESSEUR_H
#define _PROCESSEUR_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

using namespace std;

class P18f4550;

#include "datamemory.h"
#include "flashmemory.h"
#include "instructions.h"
#include "adc.h"
#include "alu.h"
#include "ports.h"
#include "timer2.h"
#include "timer0.h"
#include "timer1.h"
#include "pwm.h"
#include "evenement.h"

#include <QString>

class Timer2;

class JeuInstructions {
private :
    vector<InstructionPtr> jeu;
    P18f4550 *processeur;
public :
    JeuInstructions(P18f4550 *p);
    ~JeuInstructions();
    InstructionPtr decode(Word instr);
    InstructionPtr nop() {  return jeu[0];  }
};

class P18f4550 {
private :
    JeuInstructions jeu;
    Alu alu;
    FlashMemory fm;
    DataMemory *dm;
    int etat;

    DWord fetchPC, instrPC;
    InstructionPtr fetchPtr, instrPtr;

    PortA portA;
    PortB portB;
    PortC portC;
    PortD portD;
    PortE portE;
    ModuleADC adc;
    Timer0 *timer0;
    Timer1 *timer1;
    Timer2 *timer2;
    Pwm pwm1, pwm2;

    uint64_t temps;

    ListeEvts evts;

public :
    P18f4550();
    ~P18f4550();
    void powerOnReset();
    void powerOff();
    void stackReset();
    void mclrReset();
    int readFromHex(char *filename);
    void clock();
    int clock4();
    DataMemory *getDM() { return dm; }
    FlashMemory &getFM() { return fm; }
    Alu &getALU() { return alu; }
    Word getFetch();
    void resetFetch() {  fetchPtr = jeu.nop();  }
    PortA *getPortA() { return &portA; }
    PortB *getPortB() { return &portB; }
    PortC *getPortC() { return &portC; }
    PortD *getPortD() { return &portD; }
    PortE *getPortE() { return &portE; }
    ModuleADC *getADC() { return &adc; }
    Timer0 *getTimer0() { return timer0; }
    Timer1 *getTimer1() { return timer1; }
    Timer2 *getTimer2() { return timer2; }
    Pwm *getPwm1() { return &pwm1; }
    Pwm *getPwm2() { return &pwm2; }
    uint64_t time() { return temps; }
    ListeEvtsPtr getListeEvts() {  return &evts;  }
    Port *getPort(int n) {
        switch (n) {
        case Port::PORTA : return &portA;  break;
        case Port::PORTB : return &portB;  break;
        case Port::PORTC : return &portC;  break;
        case Port::PORTD : return &portD;  break;
        case Port::PORTE : return &portE;  break;
        }
        return nullptr;
    }
};

inline Word P18f4550::getFetch() { return fetchPtr->getInstr(); }

#endif
