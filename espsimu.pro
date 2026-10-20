#-------------------------------------------------
#
# Project created by QtCreator 2016-11-15T12:07:14
#
#-------------------------------------------------

QT       += core gui xml svg core5compat charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = espsimu
TEMPLATE = app

CONFIG += rtti
CONFIG += static

SOURCES += main.cpp\
    connexion.cpp \
    esp32backend.cpp \
    grovelcdrgb.cpp \
    mainwindow.cpp \
    lcdscreen.cpp \
    ledalpha.cpp \
    modele.cpp \
    fenetrecartemicro.cpp \
    fenetreoscillos.cpp \
    fenetrepiste.cpp \
    fenetreterminal.cpp \
    gamel.cpp \
    oscillo.cpp \
    piste.cpp \
    xmlopensavedata.cpp \
    apropos.cpp

HEADERS  += mainwindow.h \
    connexion.h \
    esp32backend.h \
    grovelcdrgb.h \
    lcdfont.h \
    lcdscreen.h \
    ledalpha.h \
    fenetrecartemicro.h \
    fenetreoscillos.h \
    fenetrepiste.h \
    fenetreterminal.h \
    gamel.h \
    modele.h \
    oscillo.h \
    piste.h \
    xmlopensavedata.h \
    apropos.h

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

TRANSLATIONS = espsimu_en.ts \
               espsimu_it.ts

QMAKE_CFLAGS_RELEASE  += -O3
QMAKE_CXXFLAGS_RELEASE  += -O3
QMAKE_LFLAGS  += -O3

!win32 {
    LIBS += -ldl -lpthread -lglib-2.0 -lpixman-1 -lz -lgcrypt -lm
    QMAKE_LFLAGS += -rdynamic
}

