#include "processeur.h"

#include "mainwindow.h"

JeuInstructions::JeuInstructions(P18f4550 *p) : processeur(p) {
    jeu.push_back(new Instruction("NOP"      , 0xFFFF, 0x0000, Instruction::NOP, p));

    jeu.push_back(new InstructionByteOriented("ADDWF"    , 0xFC00, 0x2400, Instruction::ADDWF, p));
    jeu.push_back(new InstructionByteOriented("ADDWFC"   , 0xFC00, 0x2000, Instruction::ADDWFC, p));
    jeu.push_back(new InstructionByteOriented("ANDWF"    , 0xFC00, 0x1400, Instruction::ANDWF, p));
    jeu.push_back(new InstructionByteOriented("CLRF"     , 0xFE00, 0x6A00, Instruction::CLRF, p));
    jeu.push_back(new InstructionByteOriented("COMF"     , 0xFC00, 0x1C00, Instruction::COMF, p));
    jeu.push_back(new InstructionByteOriented("CPFSEQ"   , 0xFE00, 0x6200, Instruction::CPFSEQ, p));
    jeu.push_back(new InstructionByteOriented("CPFSGT"   , 0xFE00, 0x6400, Instruction::CPFSGT, p));
    jeu.push_back(new InstructionByteOriented("CPFSLT"   , 0xFE00, 0x6000, Instruction::CPFSLT, p));
    jeu.push_back(new InstructionByteOriented("DECF"     , 0xFC00, 0x0400, Instruction::DECF, p));
    jeu.push_back(new InstructionByteOriented("DECFSZ"   , 0xFC00, 0x2C00, Instruction::DECFSZ, p));
    jeu.push_back(new InstructionByteOriented("DCFSNZ"   , 0xFC00, 0x4C00, Instruction::DCFSNZ, p));
    jeu.push_back(new InstructionByteOriented("INCF"     , 0xFC00, 0x2800, Instruction::INCF, p));
    jeu.push_back(new InstructionByteOriented("INCFSZ"   , 0xFC00, 0x3C00, Instruction::INCFSZ, p));
    jeu.push_back(new InstructionByteOriented("INFSNZ"   , 0xFC00, 0x4800, Instruction::INFSNZ, p));
    jeu.push_back(new InstructionByteOriented("IORWF"    , 0xFC00, 0x1000, Instruction::IORWF, p));
    jeu.push_back(new InstructionByteOriented("MOVF"     , 0xFC00, 0x5000, Instruction::MOVF, p));

    jeu.push_back(new InstructionMovff(p));

    jeu.push_back(new InstructionByteOriented("MOVWF"    , 0xFE00, 0x6E00, Instruction::MOVWF, p));
    jeu.push_back(new InstructionByteOriented("MULWF"    , 0xFE00, 0x0200, Instruction::MULWF, p));
    jeu.push_back(new InstructionByteOriented("NEGF"     , 0xFE00, 0x6C00, Instruction::NEGF, p));
    jeu.push_back(new InstructionByteOriented("RLCF"     , 0xFC00, 0x3400, Instruction::RLCF, p));
    jeu.push_back(new InstructionByteOriented("RLNCF"    , 0xFC00, 0x4400, Instruction::RLNCF, p));
    jeu.push_back(new InstructionByteOriented("RRCF"     , 0xFC00, 0x3000, Instruction::RRCF, p));
    jeu.push_back(new InstructionByteOriented("RRNCF"    , 0xFC00, 0x4000, Instruction::RRNCF, p));
    jeu.push_back(new InstructionByteOriented("SETF"     , 0xFE00, 0x6800, Instruction::SETF, p));
    jeu.push_back(new InstructionByteOriented("SUBFWB"   , 0xFC00, 0x5400, Instruction::SUBFWB, p));
    jeu.push_back(new InstructionByteOriented("SUBWF"    , 0xFC00, 0x5C00, Instruction::SUBWF, p));
    jeu.push_back(new InstructionByteOriented("SUBWFB"   , 0xFC00, 0x5800, Instruction::SUBWFB, p));
    jeu.push_back(new InstructionByteOriented("SWAPF"    , 0xFC00, 0x3800, Instruction::SWAPF, p));
    jeu.push_back(new InstructionByteOriented("TSTFSZ"   , 0xFE00, 0x6600, Instruction::TSTFSZ, p));
    jeu.push_back(new InstructionByteOriented("XORWF"    , 0xFC00, 0x1800, Instruction::XORWF, p));

    jeu.push_back(new InstructionBitOriented("BCF"      , 0xF000, 0x9000, Instruction::BCF, p));
    jeu.push_back(new InstructionBitOriented("BSF"      , 0xF000, 0x8000, Instruction::BSF, p));
    jeu.push_back(new InstructionBitOriented("BTFSC"    , 0xF000, 0xB000, Instruction::BTFSC, p));
    jeu.push_back(new InstructionBitOriented("BTFSS"    , 0xF000, 0xA000, Instruction::BTFSS, p));
    jeu.push_back(new InstructionBitOriented("BTG"      , 0xF000, 0x7000, Instruction::BTG, p));

    jeu.push_back(new InstructionControl8bits("BC"       , 0xFF00, 0xE200, Instruction::BC, p));
    jeu.push_back(new InstructionControl8bits("BN"       , 0xFF00, 0xE600, Instruction::BN, p));
    jeu.push_back(new InstructionControl8bits("BNC"      , 0xFF00, 0xE300, Instruction::BNC, p));
    jeu.push_back(new InstructionControl8bits("BNN"      , 0xFF00, 0xE700, Instruction::BNN, p));
    jeu.push_back(new InstructionControl8bits("BNOV"     , 0xFF00, 0xE500, Instruction::BNOV, p));
    jeu.push_back(new InstructionControl8bits("BNZ"      , 0xFF00, 0xE100, Instruction::BNZ, p));
    jeu.push_back(new InstructionControl8bits("BOV"      , 0xFF00, 0xE400, Instruction::BOV, p));
    jeu.push_back(new InstructionControl8bits("BZ"       , 0xFF00, 0xE000, Instruction::BZ, p));

    jeu.push_back(new InstructionControl11bits("BRA"      , 0xF800, 0xD000, Instruction::BRA, p));
    jeu.push_back(new InstructionControl20bits("CALL"     , 0xFE00, 0xEC00, Instruction::CALL, p));

    jeu.push_back(new Instruction("CLRWDT"   , 0xFFFF, 0x0004, Instruction::CLRWDT, p));

    jeu.push_back(new InstructionDaw(p));

    jeu.push_back(new InstructionControl20bits("GOTO"     , 0xFF00, 0xEF00, Instruction::GOTO, p));

    jeu.push_back(new Instruction("NOP"      , 0xF000, 0xF000, Instruction::NOP, p));

    jeu.push_back(new InstructionPop(p));

    jeu.push_back(new InstructionPush(p));
    jeu.push_back(new InstructionRCall(p));
    jeu.push_back(new InstructionReset(p));

    jeu.push_back(new Instruction("RETFIE"   , 0xFFFE, 0x0010, Instruction::RETFIE, p));

    jeu.push_back(new InstructionLiteral("RETLW"    , 0xFF00, 0x0C00, Instruction::RETLW, p));

    jeu.push_back(new InstructionReturn(p));

    jeu.push_back(new Instruction("SLEEP"    , 0xFFFF, 0x0003, Instruction::SLEEP, p));

    jeu.push_back(new InstructionLiteral("ADDLW"    , 0xFF00, 0x0F00, Instruction::ADDLW, p));
    jeu.push_back(new InstructionLiteral("ANDLW"    , 0xFF00, 0x0B00, Instruction::ANDLW, p));
    jeu.push_back(new InstructionLiteral("IORLW"    , 0xFF00, 0x0900, Instruction::IORLW, p));
    jeu.push_back(new InstructionLiteral("MOVLB"    , 0xFFF0, 0x0100, Instruction::MOVLB, p));
    jeu.push_back(new InstructionLiteral("MOVLW"    , 0xFF00, 0x0E00, Instruction::MOVLW, p));
    jeu.push_back(new InstructionLiteral("MULLW"    , 0xFF00, 0x0D00, Instruction::MULLW, p));
    jeu.push_back(new InstructionLiteral("SUBLW"    , 0xFF00, 0x0800, Instruction::SUBLW, p));
    jeu.push_back(new InstructionLiteral("XORLW"    , 0xFF00, 0x0A00, Instruction::XORLW, p));

    jeu.push_back(new InstructionLfsr(p));

    jeu.push_back(new InstructionTblrd("TBLRD*"   , 0xFFFF, 0x0008, Instruction::TBLRD, p));
    jeu.push_back(new InstructionTblrd("TBLRD*+"  , 0xFFFF, 0x0009, Instruction::TBLRD_P, p));
    jeu.push_back(new InstructionTblrd("TBLRD*-"  , 0xFFFF, 0x000A, Instruction::TBLRD_M, p));
    jeu.push_back(new InstructionTblrd("TBLRD+*"  , 0xFFFF, 0x000B, Instruction::TBLRD_E, p));

    jeu.push_back(new Instruction("TBLWT*"   , 0xFFFF, 0x000C, Instruction::TBLWT, p));
    jeu.push_back(new Instruction("TBLWT*+"  , 0xFFFF, 0x000D, Instruction::TBLWT_P, p));
    jeu.push_back(new Instruction("TBLWT*-"  , 0xFFFF, 0x000E, Instruction::TBLWT_M, p));
    jeu.push_back(new Instruction("TBLWT+*"  , 0xFFFF, 0x000F, Instruction::TBLWT_E, p));
}

JeuInstructions::~JeuInstructions() {
    for (unsigned int i=0; i<jeu.size(); i++) delete jeu[i];
    jeu.clear();
};

InstructionPtr JeuInstructions::decode(Word instr) {
    for (unsigned int i=0; i<jeu.size(); i++) {
        if (jeu[i]->decode(instr)) return jeu[i];
    }
    return jeu[0]; // invalid instruction -> NOP
}

P18f4550::P18f4550() :  jeu(this), alu(this),
                        fm(&jeu),
                        portA(this), portB(this), portC(this), portD(this), portE(this),
                        adc(this),
                        pwm1(this, DataMemory::CCP1CON, DataMemory::CCPR1L, DataMemory::CCPR1H, &portC, 2),
                        pwm2(this, DataMemory::CCP2CON, DataMemory::CCPR2L, DataMemory::CCPR2H, &portC, 1)
{
    temps = 0;
    dm = new DataMemory(this);
    timer0 = new Timer0(this);
    timer1 = new Timer1(this);
    timer2 = new Timer2(this);
    alu.reset();
    powerOnReset();
}

P18f4550::~P18f4550()
{
    delete timer0;
    delete timer2;
    delete dm;
}

int P18f4550::readFromHex(char *filename) {
    int res = fm.readFromHex(filename);
    powerOnReset();
    return res;
}

void P18f4550::powerOnReset() {
    timer0->reset();
    timer2->reset();
    pwm1.reset();
    pwm2.reset();
    adc.reset();
    dm->resetPC();
    dm->powerOnReset();
    etat = 0;
    fetchPC = 0;
    instrPC = 0;
    fetchPtr = jeu.nop();
    instrPtr = jeu.nop();
    dm->setW(0);
    portA.eval();
    portB.eval();
    portC.eval();
    portD.eval();
    portE.eval();
}

void P18f4550::powerOff() {
    powerOnReset();
}

void P18f4550::stackReset() {
    dm->resetPC();
    etat = 0;
    fetchPtr = jeu.nop();
    instrPtr = jeu.nop();
}

void P18f4550::mclrReset() {
    dm->resetPC();
    etat = 0;
    fetchPtr = jeu.nop();
    instrPtr = jeu.nop();
}

// CLRWDT RETFIE SLEEP TBLWT ...
void P18f4550::clock() {

    temps++;
    if (etat==0) { // Q1 1er mot
        etat = 1;
        instrPC = fetchPC;
        instrPtr = fetchPtr;
        fetchPC = dm->getPC();
        fetchPtr = fm.getInstr(fetchPC);
    } else if (etat==1) { // Q2 1er mot
        etat = 2;
        instrPtr->parametres();

//    char buf[200];
//    sprintf(buf, "%04X - %04X %-40s ** W=%02X ** FSR0=%03X->%02X ** FSR1=%03X->%02X ** FSR2=%03X->%02X ** 316->%02X *** PORTD=%02X LATD=%02X TRISD=%02X"
//            , (int)instrPC, (int)instrPtr->getInstr(), instrDec->affiche().c_str(), dm->w(),
//            (((int)dm->read(dm->FSR0H))*256+(int)dm->read(dm->FSR0L)), (int)dm->read(dm->INDF0),
//            (((int)dm->read(dm->FSR1H))*256+(int)dm->read(dm->FSR1L)), (int)dm->read(dm->INDF1),
//            (((int)dm->read(dm->FSR2H))*256+(int)dm->read(dm->FSR2L)), (int)dm->read(dm->INDF2),
//            (int)dm->read(0x316)
//            ,(int)dm->read(dm->PORTD), (int)dm->read(dm->LATD), (int)dm->read(dm->TRISD));
//    (*dbg) << buf << endl;

    } else if (etat==2) { // Q3 1er mot
        etat = 3;
        instrPtr->process();
    } else if (etat==3) { // Q4 1er mot
        etat = (instrPtr->execute()) ? 0 : 4;  // instructions speciales de 2 cycles : MOVFF envoie en etat -> 4
        dm->nextPC();
    } else if (etat==4) { // Q1 2eme mot
        fetchPC = dm->getPC();
        fetchPtr = fm.getInstr(fetchPC);
        etat = 5;
    } else if (etat==5) { // Q2 2eme mot
        etat = 6;
        instrPtr->specialQ2();
    } else if (etat==6) { // Q3 2eme mot
        etat = 7;
    } else if (etat==7) { // Q4 2eme mot
        etat = 0;
        instrPtr->specialQ4();
        dm->nextPC();
    }
    evts.executer();
//    adc.clock();
//    timer2.clock();
    portA.eval();
    portB.eval();
    portC.eval();
    portD.eval();
    portE.eval();
}


int P18f4550::clock4() {
    temps++;
    etat = 1;
    instrPC = fetchPC;
    instrPtr = fetchPtr;
    fetchPC = dm->getPC();
    fetchPtr = fm.getInstr(fetchPC);

    evts.executer();
    portA.eval();
    portB.eval();
    portC.eval();
    portD.eval();
    portE.eval();

    temps++;
    instrPtr->parametres();

    evts.executer();
    portA.eval();
    portB.eval();
    portC.eval();
    portD.eval();
    portE.eval();

    temps++;
    instrPtr->process();

    evts.executer();
    portA.eval();
    portB.eval();
    portC.eval();
    portD.eval();
    portE.eval();

    temps++;
    etat = (instrPtr->execute()) ? 0 : 4;  // instructions speciales de 2 cycles : MOVFF envoie en etat -> 4
    dm->nextPC();

    evts.executer();
    portA.eval();
    portB.eval();
    portC.eval();
    portD.eval();
    portE.eval();

    if (etat==0) return 4; // nombre de cycles exécutés

    temps++;
    fetchPC = dm->getPC();
    fetchPtr = fm.getInstr(fetchPC);

    evts.executer();
    portA.eval();
    portB.eval();
    portC.eval();
    portD.eval();
    portE.eval();

    temps++;
    instrPtr->specialQ2();

    evts.executer();
    portA.eval();
    portB.eval();
    portC.eval();
    portD.eval();
    portE.eval();

    temps++;

    evts.executer();
    portA.eval();
    portB.eval();
    portC.eval();
    portD.eval();
    portE.eval();

    temps++;
    instrPtr->specialQ4();
    dm->nextPC();

    evts.executer();
    portA.eval();
    portB.eval();
    portC.eval();
    portD.eval();
    portE.eval();

    return 8;
}

