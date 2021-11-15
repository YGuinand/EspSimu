#include "instructions.h"
#include "processeur.h"

Instruction::Instruction(string n, Word m, Word o, int c, P18f4550 *p) : processeur(p) {
    name = n;
    mask = m;
    opcode = o;
    code = c;
}

bool Instruction::decode(Word i) {
    instr = i;
    return ((i & mask) == opcode);
}

bool Instruction::execute() {
/*    if (code) {
        cout << "ERREUR : Instruction non implementee !\n";
    }*/
    return true;
};

void InstructionByteOriented::preParametres() {
    d = (instr & 0x0200) != 0;
    a = (instr & 0x0100) != 0;
    f8 = instr & 0x00FF;
};

void InstructionByteOriented::parametres() {
    processeur->getDM()->doPreIndirect(a, f8);
    switch (code) {
    case CLRF :
    case MOVWF :
    case SETF :
        break;
    default :
        fReg = processeur->getDM()->read(a, f8);
    }
};

void InstructionBitOriented::preParametres() {
    b = (instr & 0x0E00) >> 9;
    a = (instr & 0x0100) != 0;
    f8 = instr & 0x00FF;
};

void InstructionBitOriented::parametres() {
    processeur->getDM()->doPreIndirect(a, f8);
    fReg = processeur->getDM()->read(a, f8);
};

void InstructionControl20bits::preParametres() {
    s = (instr & 0x0100) != 0;
};

void InstructionControl20bits::parametres() {
    n = processeur->getFetch() & 0x0FFF;
    n = (n << 8) + (instr & 0x00FF);
};

void InstructionMovff::preParametres() {
    fsrc = instr & 0x0FFF;
};

void InstructionMovff::parametres() {
    fdest = processeur->getFetch() & 0x0FFF;
    processeur->getDM()->doPreIndirect(fdest);
    processeur->getDM()->doPreIndirect(fsrc);
    fReg = processeur->getDM()->read(fsrc);
};

void InstructionReset::parametres() {
    processeur->mclrReset();
};

void InstructionPush::parametres() {
    Byte config4L;
    processeur->getFM().readByte((DWord)0x300006, config4L); // lire STVREN
    if (processeur->getDM()->push(processeur->getDM()->getPC() + 2, config4L&0x01)) processeur->stackReset();
};

void InstructionRCall::parametres() {
    InstructionControl11bits::parametres();
    Byte config4L;
    processeur->getFM().readByte((DWord)0x300006, config4L); // lire STVREN
    if (processeur->getDM()->push(processeur->getDM()->getPC(), config4L&0x01)) processeur->stackReset();
};

void InstructionLfsr::preParametres() {
    kH = instr & 0x000F;
    f = (instr & 0x0030) >> 4;
}

void InstructionLfsr::parametres() {
    kL = processeur->getFetch() & 0x00FF;
}

void InstructionLiteral::process() {
    switch (code) {
    case MOVLB :
    case MOVLW :
    case MULLW :
        break;
    case RETLW :
        processeur->getALU().operationMOV(Alu::MASK_NONE, k);
        break;
    case ADDLW :
        processeur->getALU().operationADD(Alu::MASK_ALL, k);
        break;
    case ANDLW :
        processeur->getALU().operationAND(Alu::MASK_ZN, k);
        break;
    case IORLW :
        processeur->getALU().operationOR(Alu::MASK_ZN, k);
        break;
    case SUBLW :
        processeur->getALU().operationSUB(Alu::MASK_ALL, k);
        break;
    case XORLW :
        processeur->getALU().operationXOR(Alu::MASK_ZN, k);
        break;
    }
}

void InstructionByteOriented::process() {
    switch (code) {
    case MULWF :
        break;
    case ADDWF :
        processeur->getALU().operationADD(Alu::MASK_ALL, fReg);
        break;
    case ADDWFC :
        processeur->getALU().operationADD(Alu::MASK_ALL, fReg, true);
        break;
    case ANDWF :
        processeur->getALU().operationAND(Alu::MASK_ZN, fReg);
        break;
    case CLRF :
        processeur->getALU().operationCLR(Alu::MASK_Z);
        break;
    case COMF :
        processeur->getALU().operationCOM(Alu::MASK_ZN, fReg);
        break;
    case CPFSEQ :
        condition = fReg == processeur->getDM()->w();
        break;
    case CPFSGT :
        condition = fReg > processeur->getDM()->w();
        break;
    case CPFSLT :
        condition = fReg < processeur->getDM()->w();
        break;
    case DECF :
        processeur->getALU().operationDEC(Alu::MASK_ALL, fReg);
        break;
    case DECFSZ :
        processeur->getALU().operationDEC(Alu::MASK_NONE, fReg);
        condition = processeur->getALU().isZero();
        break;
    case DCFSNZ :
        processeur->getALU().operationDEC(Alu::MASK_NONE, fReg);
        condition = !processeur->getALU().isZero();
        break;
    case INCF :
        processeur->getALU().operationINC(Alu::MASK_ALL, fReg);
        break;
    case INCFSZ :
        processeur->getALU().operationINC(Alu::MASK_NONE, fReg);
        condition = processeur->getALU().isZero();
        break;
    case INFSNZ :
        processeur->getALU().operationINC(Alu::MASK_NONE, fReg);
        condition = !processeur->getALU().isZero();
        break;
    case IORWF :
        processeur->getALU().operationOR(Alu::MASK_ZN, fReg);
        break;
    case MOVF :
        processeur->getALU().operationMOV(Alu::MASK_ZN, fReg);
        break;
    case MOVWF :
        processeur->getALU().operationMOVW(Alu::MASK_NONE);
        break;
    case NEGF :
        processeur->getALU().operationNEG(Alu::MASK_ALL, fReg);
        d = 1;
        break;
    case RLCF :
        processeur->getALU().operationLEFT(Alu::MASK_CZN, fReg, true);
        break;
    case RLNCF :
        processeur->getALU().operationLEFT(Alu::MASK_ZN, fReg);
        break;
    case RRCF :
        processeur->getALU().operationRIGHT(Alu::MASK_CZN, fReg, true);
        break;
    case RRNCF :
        processeur->getALU().operationRIGHT(Alu::MASK_ZN, fReg);
        break;
    case SETF :
        processeur->getALU().operationSET(Alu::MASK_NONE);
        d = 1;
        break;
    case SUBFWB :
        processeur->getALU().operationSUBW(Alu::MASK_ALL, fReg, true);
        break;
    case SUBWF :
        processeur->getALU().operationSUB(Alu::MASK_ALL, fReg);
        break;
    case SUBWFB :
        processeur->getALU().operationSUB(Alu::MASK_ALL, fReg, true);
        break;
    case SWAPF :
        processeur->getALU().operationSWAP(Alu::MASK_NONE, fReg);
        break;
    case TSTFSZ :
        condition = fReg == 0;
        break;
    case XORWF :
        processeur->getALU().operationXOR(Alu::MASK_ZN, fReg);
        break;
    }
}

void InstructionBitOriented::process() {
    switch (code) {
    case BCF :
        processeur->getALU().operationMOV(Alu::MASK_NONE, fReg & ~(1 << b));
        break;
    case BSF :
        processeur->getALU().operationMOV(Alu::MASK_NONE, fReg | (1 << b));
        break;
    case BTFSC :
        condition = (fReg & (1 << b)) == 0;
        break;
    case BTFSS :
        condition = (fReg & (1 << b)) != 0;
        break;
    case BTG :
        processeur->getALU().operationMOV(Alu::MASK_NONE, fReg ^ (1 << b));
        break;
    }
}

void InstructionControl8bits::process() {
    newPC = processeur->getDM()->addPC(2 * n - 2);
}

void InstructionControl20bits::process() {
    newPC = (n << 1) - 2;
    switch (code) {
    case GOTO :
        break;
    case CALL :
        Byte config4L;
        processeur->getFM().readByte((DWord)0x300006, config4L);
        if (processeur->getDM()->push(processeur->getDM()->getPC() + 2, config4L&0x01)) processeur->stackReset();
        else if (s) processeur->getDM()->pushReg();
        break;
    }
}

void InstructionReturn::process() {
    if (s) processeur->getDM()->popReg();
}

void InstructionPop::process() {
    processeur->getDM()->pop();
}

/*
void Instruction NON IMPLEMENTE ::process() {
        switch (code) {
        case CLRWDT :
            break;
        case RETFIE :
            break;
        case SLEEP :
            break;
        case TBLWT :
            break;
        case TBLWT_P :
            break;
        case TBLWT_M :
            break;
        case TBLWT_E :
            break;
        }
}
*/

bool InstructionLiteral::execute() {
    switch (code) {
    case RETLW :
        processeur->getDM()->setW(processeur->getALU().read());
        processeur->getDM()->setPC(processeur->getDM()->pop()-2);
        processeur->resetFetch(); // next instruction -> NOP
        break;
    case MOVLW :
        processeur->getDM()->setW(k);
        break;
    case ADDLW :
    case ANDLW :
    case IORLW :
    case SUBLW :
    case XORLW :
        processeur->getDM()->setW(processeur->getALU().read());
        processeur->getALU().writeStatus();
        break;
    case MOVLB :
        processeur->getDM()->writeBSR(k);
        break;
    case MULLW :
        processeur->getDM()->produit((Word)k*(Word)processeur->getDM()->w());
        break;
    }
    return true;
}

bool InstructionByteOriented::execute() {
    switch (code) {
    case ADDWF :
    case ADDWFC :
    case ANDWF :
    case CLRF :
    case COMF :
    case DECF :
    case INCF :
    case IORWF :
    case MOVF :
    case MOVWF :
    case NEGF :
    case RLCF :
    case RLNCF :
    case RRCF :
    case RRNCF :
    case SETF :
    case SUBFWB :
    case SUBWF :
    case SUBWFB :
    case SWAPF :
    case XORWF :
        if (d) processeur->getDM()->write(a, f8, processeur->getALU().read());
        else processeur->getDM()->setW(processeur->getALU().read());
        processeur->getALU().writeStatus();
        break;
    case MULWF :
        processeur->getDM()->produit((Word)fReg*(Word)processeur->getDM()->w());
        break;
    case DECFSZ :
    case DCFSNZ :
    case INCFSZ :
    case INFSNZ :
        if (d) processeur->getDM()->write(a, f8, processeur->getALU().read());
        else processeur->getDM()->setW(processeur->getALU().read());
        if (condition) {
        //    processeur->getDM().nextPC();
            processeur->resetFetch();
        }
        break;
    case CPFSEQ :
    case CPFSGT :
    case CPFSLT :
    case TSTFSZ :
        if (condition) {
        //    processeur->getDM().nextPC();
            processeur->resetFetch();
        }
        break;
    }
    processeur->getDM()->doPostIndirect(a, f8);
    return true;
}

bool InstructionBitOriented::execute() {
    switch (code) {
    case BCF :
    case BSF :
    case BTG :
        processeur->getDM()->bitSetClr(a, f8, (processeur->getALU().read() & (1<<b)) != 0, b);
        break;
    case BTFSC :
    case BTFSS :
        if (condition) processeur->resetFetch();
        break;
    }
    processeur->getDM()->doPostIndirect(a, f8);
    return true;
}

bool InstructionControl8bits::execute() {
    bool test;

    switch (code) {
    case BC :
        test = processeur->getDM()->carry();
        break;
    case BN :
        test = processeur->getDM()->negative();
        break;
    case BNC :
        test = !processeur->getDM()->carry();
        break;
    case BNN :
        test = !processeur->getDM()->negative();
        break;
    case BNOV :
        test = !processeur->getDM()->overflow();
        break;
    case BNZ :
        test = !processeur->getDM()->zero();
        break;
    case BOV :
        test = processeur->getDM()->overflow();
        break;
    case BZ :
        test = processeur->getDM()->zero();
        break;
    default :
        test = false;
    }
    if (test) {
        processeur->getDM()->setPC(newPC);
        processeur->resetFetch();
    }
    return true;
}

bool InstructionControl11bits::execute() {
    processeur->getDM()->setPC(newPC);
    processeur->resetFetch(); // nop
    return true;
}

bool InstructionControl20bits::execute() {
    processeur->getDM()->setPC(newPC);
    processeur->resetFetch(); // nop
    return true;
}

bool InstructionReturn::execute() {
    processeur->getDM()->setPC(processeur->getDM()->pop()-2);
    processeur->resetFetch();
    return true;
}

bool InstructionLfsr::execute() {
    processeur->getDM()->writeFSRfH(f, kH);
    return false;   // instruction de 2 cycles, 2nd cycle n'est pas NOP
}

bool InstructionDaw::execute() {
    Byte w = processeur->getDM()->w();
    Byte wl = w & 0x0F;
    Byte wh = (w & 0xF0) >> 4;
    Byte dc = processeur->getDM()->digitCarry();
    bool c = false;

    if ((wl > 9) || (dc)) {
        wl += 6;
        wl &= 0x0F;
    }
    c = (wh + dc > 9) || (processeur->getDM()->carry());
    if (c) {
        wh += 6 + dc;
        wh &= 0x0F;
    }
    processeur->getDM()->setW((wh<<4)+wl);
    if (c) processeur->getDM()->setCarry();
    processeur->getDM()->setDigitCarry(false);
    return true;
}

bool InstructionTblrd::execute() {
    processeur->getDM()->prevPC();
    return false;
}

/*
bool Instruction NON IMPLEMENTE ::execute()
{
 switch (code) {
        case CLRWDT :
            break;
        case RETFIE :
            break;
        case SLEEP :
            break;
        case TBLWT :
            break;
        case TBLWT_P :
            break;
        case TBLWT_M :
            break;
        case TBLWT_E :
            break;
        }
        return true;
}
*/

void InstructionMovff::specialQ4() {
    processeur->getDM()->write(fdest, fReg);
    processeur->getDM()->doPostIndirect(fdest);
    processeur->getDM()->doPostIndirect(fsrc);
}

void InstructionLfsr::specialQ4() {
    processeur->getDM()->writeFSRfL(f, kL);
}

void InstructionTblrd::specialQ4() {
    DWord adr;
    Byte b;

    if (code == TBLRD_E) processeur->getDM()->incTBLPTR();
    adr = processeur->getDM()->readTBLPTR();
    processeur->getFM().readByte(adr, b);
    processeur->getDM()->writeTABLAT(b);
    if (code == TBLRD_P) processeur->getDM()->incTBLPTR();
    if (code == TBLRD_M) processeur->getDM()->decTBLPTR();
}

string InstructionByteOriented::affiche() const {
    stringstream oss;
    oss << mnemo() << " 0x" << hex << (int)f8 << ", " << (int)d << ", " << (int)a << "  " << processeur->getDM()->name(a, f8) << " --- " << (int)fReg;
    return oss.str();
};

string InstructionBitOriented::affiche() const {
    stringstream oss;
    oss << mnemo() << " 0x" << hex << (int)f8 << ", " << dec << (int)b << ", " << (int)a << "  " << processeur->getDM()->name(a, f8);
    return oss.str();
};
