#-------------------------------------------------
#
# Project created by QtCreator 2016-11-15T12:07:14
#
#-------------------------------------------------

QT       += core gui xml svg core5compat charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = picsimu
TEMPLATE = app

CONFIG += rtti
CONFIG += static

SOURCES += main.cpp\
        mainwindow.cpp \
    instructions.cpp \
    lcdscreen.cpp \
    ledalpha.cpp \
    modele.cpp \
    fenetrecartemicro.cpp \
    fenetreoscillos.cpp \
    fenetrepiste.cpp \
    flashmemory.cpp \
    gamel.cpp \
    hex_file.cpp \
    oscillo.cpp \
    piste.cpp \
    ports.cpp \
    processeur.cpp \
    pwm.cpp \
    registre.cpp \
    timer0.cpp \
    timer1.cpp \
    timer2.cpp \
    xmlopensavedata.cpp \
    adc.cpp \
    alu.cpp \
    apropos.cpp \
    datamemory.cpp \
    evenement.cpp

HEADERS  += mainwindow.h \
    adc.h \
    hex_file.h \
    instructions.h \
    lcdfont.h \
    lcdscreen.h \
    ledalpha.h \
    fenetrecartemicro.h \
    fenetreoscillos.h \
    fenetrepiste.h \
    flashmemory.h \
    gamel.h \
    general.h \
    modele.h \
    oscillo.h \
    piste.h \
    ports.h \
    processeur.h \
    pwm.h \
    registre.h \
    timer0.h \
    timer1.h \
    timer2.h \
    xmlopensavedata.h \
    alu.h \
    apropos.h \
    datamemory.h \
    evenement.h

FORMS    += \
    apropos.ui \
    fenetrecartemicro.ui \
    fenetreoscillos.ui \
    fenetrepiste.ui \
    gamel.ui \
    mainwindow.ui

RESOURCES += \
    ressources.qrc

RC_ICONS = images/micropro.ico

TRANSLATIONS = picsimu_en.ts \
               picsimu_it.ts

QMAKE_CFLAGS_RELEASE  += -O3
QMAKE_CXXFLAGS_RELEASE  += -O3
QMAKE_LFLAGS  += -O3

