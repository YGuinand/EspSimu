
#include "datamemory.h"

const string DataMemory::nameSFR[160] = {"—",
                                        "—",
                                        "SPPDATA",
                                        "SPPCFG",
                                        "SPPEPS",
                                        "SPPCON",
                                        "UFRML",
                                        "UFRMH",
                                        "UIR",
                                        "UIE",
                                        "UEIR",
                                        "UEIE",
                                        "USTAT",
                                        "UCON",
                                        "UADDR",
                                        "UCFG",
                                        "UEP0",
                                        "UEP1",
                                        "UEP2",
                                        "UEP3",
                                        "UEP4",
                                        "UEP5",
                                        "UEP6",
                                        "UEP7",
                                        "UEP8",
                                        "UEP9",
                                        "UEP10",
                                        "UEP11",
                                        "UEP12",
                                        "UEP13",
                                        "UEP14",
                                        "UEP15",
                                        "PORTA",
                                        "PORTB",
                                        "PORTC",
                                        "PORTD",
                                        "PORTE",
                                        "—",
                                        "—",
                                        "—",
                                        "—",
                                        "LATA",
                                        "LATB",
                                        "LATC",
                                        "LATD",
                                        "LATE",
                                        "—",
                                        "—",
                                        "—",
                                        "—",
                                        "TRISA",
                                        "TRISB",
                                        "TRISC",
                                        "TRISD",
                                        "TRISE",
                                        "—",
                                        "—",
                                        "—",
                                        "—",
                                        "OSCTUNE",
                                        "—",
                                        "PIE1",
                                        "PIR1",
                                        "IPR1",
                                        "PIE2",
                                        "PIR2",
                                        "IPR2",
                                        "—",
                                        "—",
                                        "—",
                                        "EECON1",
                                        "EECON2",
                                        "EEDATA",
                                        "EEADR",
                                        "—",
                                        "RCSTA",
                                        "TXSTA",
                                        "TXREG",
                                        "RCREG",
                                        "SPBRG",
                                        "SPBRGH",
                                        "T3CON",
                                        "TMR3L",
                                        "TMR3H",
                                        "CMCON",
                                        "CVRCON",
                                        "ECCP1AS",
                                        "ECCP1DEL",
                                        "BAUDCON",
                                        "—",
                                        "CCP2CON",
                                        "CCPR2L",
                                        "CCPR2H",
                                        "CCP1CON",
                                        "CCPR1L",
                                        "CCPR1H",
                                        "ADCON2",
                                        "ADCON1",
                                        "ADCON0",
                                        "ADRESL",
                                        "ADRESH",
                                        "SSPCON2",
                                        "SSPCON1",
                                        "SSPSTAT",
                                        "SSPADD",
                                        "SSPBUF",
                                        "T2CON",
                                        "PR2",
                                        "TMR2",
                                        "T1CON",
                                        "TMR1L",
                                        "TMR1H",
                                        "RCON",
                                        "WDTCON",
                                        "HLVDCON",
                                        "OSCCON",
                                        "—",
                                        "T0CON",
                                        "TMR0L",
                                        "TMR0H",
                                        "STATUS",
                                        "FSR2L",
                                        "FSR2H",
                                        "PLUSW2",
                                        "PREINC2",
                                        "POSTDEC2",
                                        "POSTINC2",
                                        "INDF2",
                                        "BSR",
                                        "FSR1L",
                                        "FSR1H",
                                        "PLUSW1",
                                        "PREINC1",
                                        "POSTDEC1",
                                        "POSTINC1",
                                        "INDF1",
                                        "WREG",
                                        "FSR0L",
                                        "FSR0H",
                                        "PLUSW0",
                                        "PREINC0",
                                        "POSTDEC0",
                                        "POSTINC0",
                                        "INDF0",
                                        "INTCON3",
                                        "INTCON2",
                                        "INTCON",
                                        "PRODL",
                                        "PRODH",
                                        "TABLAT",
                                        "TBLPTRL",
                                        "TBLPTRH",
                                        "TBLPTRU",
                                        "PCL",
                                        "PCLATH",
                                        "PCLATU",
                                        "STKPTR",
                                        "TOSL",
                                        "TOSH",
                                        "TOSU"
                                        };

DataMemory::DataMemory(P18f4550 *p) {
    int i;

    processeur = p;
    for (i=0; i<0x800; i++) reg[i] = new Registre();
    for (i=0x800; i<0xF60; i++) reg[i] = new RegistreInutilise();
    for (i=0xF60; i<0x1000; i++) reg[i] = new RegistreSpecial(nameSFR[i-0xF60]);
    modifie(STATUS, status = new RegistreStatus());
    modifie(BSR, bsr = new RegistreBSR());
    modifie(TRISA, new RegistreTrisLat("TRISA", processeur->getPortA()));
    modifie(LATA, new RegistreTrisLat("LATA", processeur->getPortA()));
    modifie(PORTA, new RegistrePort("PORTA", reg[LATA], processeur->getPortA()));
    modifie(TRISB, new RegistreTrisLat("TRISB", processeur->getPortB()));
    modifie(LATB, new RegistreTrisLat("LATB", processeur->getPortB()));
    modifie(PORTB, new RegistrePort("PORTB", reg[LATB], processeur->getPortB()));
    modifie(TRISC, new RegistreTrisLat("TRISC", processeur->getPortC()));
    modifie(LATC, new RegistreTrisLat("LATC", processeur->getPortC()));
    modifie(PORTC, new RegistrePort("PORTC", reg[LATC], processeur->getPortC()));
    modifie(TRISD, new RegistreTrisLat("TRISD", processeur->getPortD()));
    modifie(LATD, new RegistreTrisLat("LATD", processeur->getPortD()));
    modifie(PORTD, new RegistrePort("PORTD", reg[LATD], processeur->getPortD()));
    modifie(TRISE, new RegistreTrisLat("TRISE", processeur->getPortE()));
    modifie(LATE, new RegistreTrisLat("LATE", processeur->getPortE()));
    modifie(PORTE, new RegistrePort("PORTE", reg[LATE], processeur->getPortE()));
    modifie(PREINC0, new RegistrePREINC0(this));
    modifie(PREINC1, new RegistrePREINC1(this));
    modifie(PREINC2, new RegistrePREINC2(this));
    modifie(POSTINC0, new RegistrePOSTINC0(this));
    modifie(POSTINC1, new RegistrePOSTINC1(this));
    modifie(POSTINC2, new RegistrePOSTINC2(this));
    modifie(POSTDEC0, new RegistrePOSTDEC0(this));
    modifie(POSTDEC1, new RegistrePOSTDEC1(this));
    modifie(POSTDEC2, new RegistrePOSTDEC2(this));
    modifie(INDF0, new RegistreIndirect0("INDF0", this));
    modifie(INDF1, new RegistreIndirect1("INDF1", this));
    modifie(INDF2, new RegistreIndirect2("INDF2", this));
    modifie(PLUSW0, new RegistrePLUSW0(this));
    modifie(PLUSW1, new RegistrePLUSW1(this));
    modifie(PLUSW2, new RegistrePLUSW2(this));
    modifie(ADCON0, new RegistreADCON0(processeur->getADC()));
    modifie(ADCON1, new RegistreADCON1(processeur->getADC()));
    modifie(ADCON2, new RegistreADCON2(processeur->getADC()));
    modifie(CCP1CON, new RegistreCCPxCON("CCP1CON", processeur->getPwm1()));
    modifie(CCPR1L, new RegistreCCPRxL("CCPR1L", processeur->getPwm1()));
    modifie(CCPR1H, new RegistreCCPRxH("CCPR1H", processeur->getPwm1()));
    modifie(CCP2CON, new RegistreCCPxCON("CCP2CON", processeur->getPwm2()));
    modifie(CCPR2L, new RegistreCCPRxL("CCPR2L", processeur->getPwm2()));
    modifie(CCPR2H, new RegistreCCPRxH("CCPR2H", processeur->getPwm2()));
}

DataMemory::~DataMemory() {
    for (int i=0; i<0x1000; i++) delete reg[i];
}

void DataMemory::powerOnReset()
{
    for (int i=0; i<32; i++) stack[i]=0;
    for (int i=0; i<0x1000; i++) reg[i]->powerOnReset();
    reg[TRISA]->write(0xFF);
    reg[TRISB]->write(0xFF);
    reg[TRISC]->write(0xFF);
    reg[TRISD]->write(0xFF);
    reg[TRISE]->write(0xFF);
    reg[ADCON0]->write(0x00);
    reg[ADCON1]->write((processeur->getFM().bitPBADEN()) ? 0x00 : 0x07);
    reg[ADCON2]->write(0x00);
}

string DataMemory::name(Word adr) {
    if (adr>=0x1000) return "OUT OF MEMORY";
    return reg[adr]->name();
}

Byte DataMemory::read(bool a, Byte adrL) {
    Word adr;
    if (a) {
        adr = readBSR();
        adr = (adr << 8) + adrL;
        return read(adr);
    }
    return readAccessBank(adrL);
}

void DataMemory::write(bool a, Byte adrL, Byte b) {
    Word adr;
    if (a) {
        adr = readBSR();
        adr = (adr << 8) + adrL;
        write(adr, b);
    } else writeAccessBank(adrL, b);
}

void DataMemory::bitSetClr(bool a, Byte adrL, bool b, Byte num) {
    Word adr;
    if (a) {
        adr = readBSR();
        adr = (adr << 8) + adrL;
        bitSetClr(adr, b, num);
    } else setBitAccessBank(adrL, b, num);
}

bool DataMemory::push(DWord pc, bool stvren) {
    Byte stackPtrReg = reg[STKPTR]->readDirect();
    Byte stackPtr = stackPtrReg & 0x1F;
    Byte stackFull = stackPtrReg & 0x80;
    Byte stackUnd = stackPtrReg & 0x40;

    pc = pc & 0x1FFFFF;
    if (stackPtr < 31) {
        stackPtr++;
        if (stackPtr == 31) {
            stackFull = 0x80;
            if (stvren) {
                stack[31] = programCounter + 2;
                reg[TOSU]->writeDirect((stack[31] & 0x1F0000) >> 16);    // TOSU
                reg[TOSH]->writeDirect((stack[31] & 0x00FF00) >> 8);     // TOSH
                reg[TOSL]->writeDirect(stack[31] & 0x0000FF);           // TOSL
                reg[STKPTR]->writeDirect(stackFull | stackUnd);
                return true;     // full stack reset
            }
        }
        stack[stackPtr] = pc;
        reg[TOSU]->writeDirect((pc & 0x1F0000) >> 16);    // TOSU
        reg[TOSH]->writeDirect((pc & 0x00FF00) >> 8);     // TOSH
        reg[TOSL]->writeDirect(pc & 0x0000FF);           // TOSL
        reg[STKPTR]->writeDirect(stackPtr | stackFull | stackUnd);
    }
    return false;  // no reset
}

DWord DataMemory::pop() {
    Byte stackPtrReg = reg[STKPTR]->readDirect();
    Byte stackPtr = stackPtrReg & 0x1F;
    Byte stackFull = stackPtrReg & 0x80;
    Byte stackUnd = stackPtrReg & 0x40;
    DWord stackVal= stack[stackPtr];

    if (stackPtr > 0) stackPtr--;
    if (stackPtr == 0) stackUnd = 0x40;
    reg[TOSU]->writeDirect((stack[stackPtr] & 0x1F0000) >> 16);    // TOSU
    reg[TOSH]->writeDirect((stack[stackPtr] & 0x00FF00) >> 8);     // TOSH
    reg[TOSL]->writeDirect(stack[stackPtr] & 0x0000FF);           // TOSL
    reg[STKPTR]->writeDirect(stackPtr | stackFull | stackUnd);
    return stackVal;
}

void DataMemory::popReg(void) {
    write(STATUS, statusS);
    write(BSR, bsrS);
    write(WREG, wS);
}

void DataMemory::produit(Word prod) {
    reg[PRODL]->writeDirect(prod & 0x00FF);           // PRODL
    reg[PRODH]->writeDirect((prod & 0xFF00) >> 8);    // PRODH
}

void DataMemory::writeFSRfH(Byte f, Byte b) {
    switch (f) {
    case 0 :
        reg[FSR0H]->writeDirect(b&0x0F);
        break;
    case 1 :
        reg[FSR1H]->writeDirect(b&0x0F);
        break;
    case 2 :
        reg[FSR2H]->writeDirect(b&0x0F);
        break;
    }
}

void DataMemory::writeFSRfL(Byte f, Byte b) {
    switch (f) {
    case 0 :
        reg[FSR0L]->writeDirect(b);
        break;
    case 1 :
        reg[FSR1L]->writeDirect(b);
        break;
    case 2 :
        reg[FSR2L]->writeDirect(b);
        break;
    }
}

DWord DataMemory::readTBLPTR() {
    DWord adr = reg[TBLPTRU]->readDirect() & 0x3F;
    adr = (adr << 8) + reg[TBLPTRH]->readDirect();
    adr = (adr << 8) + reg[TBLPTRL]->readDirect();
    if (adr & 0x00200000) { // accès à la zone config
        adr = adr - 0x00200000 + 0x00300000;
    }
    return adr;
}

void DataMemory::incTBLPTR() {
    reg[TBLPTRL]->incDirect();
    if (reg[TBLPTRL]->readDirect() == 0) {
        reg[TBLPTRH]->incDirect();
        if (reg[TBLPTRH]->readDirect() == 0) {
            bool config = reg[TBLPTRU]->readDirect() & 0x20;
            reg[TBLPTRU]->writeDirect((reg[TBLPTRU]->readDirect()+1) & 0x1F);
            if (config) reg[TBLPTRU]->incDirect(0x20);
        }
    }
}

void DataMemory::decTBLPTR() {
    reg[TBLPTRL]->decDirect();
    if (reg[TBLPTRL]->readDirect() == 0xFF) {
        reg[TBLPTRH]->decDirect();
        if (reg[TBLPTRH]->readDirect() == 0xFF) {
            bool config = reg[TBLPTRU]->readDirect() & 0x20;
            reg[TBLPTRU]->writeDirect((reg[TBLPTRU]->readDirect()-1) & 0x1F);
            if (config) reg[TBLPTRU]->incDirect(0x20); // Pourquoi cette addition ?
        }
    }
}

Word DataMemory::getAdr(bool a, Byte adrL) {
    Word adr;
    if (a) {
        adr = readBSR();
        adr = (adr << 8) + adrL;
        return adr;
    }
    if (adrL<0x60) return adrL;
    return adrL+0xF00;
}



