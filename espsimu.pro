#-------------------------------------------------
#
# Project created by QtCreator 2016-11-15T12:07:14
#
#-------------------------------------------------

QT       += core gui xml svg core5compat charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = espsimu
TEMPLATE = app

# ================================================================
# Version de l'application : source unique, diffusee deux fois
#   - dans la ressource de l'exe Windows (espsimu_resource.rc)
#   - dans tout le C++ via la macro APP_VERSION (litteral "2.10"),
#     utilisee par mainwindow.cpp (titre fenetre principale) et
#     apropos.cpp (jeton %APPVERSION% de la fenetre A propos)
# ================================================================
APP_VERSION = 2.10

win32: VERSION = $${APP_VERSION}
DEFINES += APP_VERSION=\\\"$${APP_VERSION}\\\"

CONFIG += rtti
CONFIG += static

SOURCES += main.cpp\
    connexion.cpp \
    diagnostic.cpp \
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
    parametres.cpp \
    xmlopensavedata.cpp \
    apropos.cpp

HEADERS  += mainwindow.h \
    batterie.h \
    pontdiv.h \
    connexion.h \
    diagnostic.h \
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
    parametres.h \
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

# build wasm : branche POC A (stub) puis POC C (backend QEMU-wasm statique).
# POC C M2 : le module embarque QEMU 10.1 wasm (emcc 4.0.7) produit par M1 —
#   - libqemu-all.a : tous les .o de la cible qemu-system-xtensa.js
#     (system_main.c.o EXCLU, pas de main() QEMU en collision) + libqemuutil + libfdt ;
#   - deps ktock transposees : exactement la liste du link meson M1
#     (libffi, libglib-2.0, libpcre2-8, libz — ni gio, ni pixman) ;
#   - drapeaux emcc : memoire partagee FIXE (SAB pthreads, croissance
#     interdite), FORCE_FILESYSTEM (MEMFS pour /bios + /espsimu_flash.bin),
#     pool de workers > 4 (qemu_init cree plusieurs threads internes) ;
#     ni ASYNCIFY ni PROXY_TO_PTHREAD (main() reste le thread Qt).
wasm {
    # pas de libs natives desktop (dlopen/pthread explicites, gcrypt…) :
    # pthread arrive par -pthread, dl n'existe pas sur wasm.
    LIBS -= -ldl -lpthread -lglib-2.0 -lpixman-1 -lz -lgcrypt -lm
    QMAKE_LFLAGS -= -rdynamic

    SOURCES -= esp32backend.cpp
    SOURCES += esp32backend_wasm.cpp
    RESOURCES += wasmroms.qrc

    # Surcharge possible sans editer le .pro : QEMU_WASM_HOME=<arbre> qmake ...
    # (arbre de build QEMU-wasm, produit par recettes-wasm/m1-emsdk407.sh de
    # la branche poc-b du depot qemu/)
    QEMU_WASM_HOME = $$(QEMU_WASM_HOME)
    isEmpty(QEMU_WASM_HOME): QEMU_WASM_HOME = /home/yves/tmp/qemu-wasm-407
    QEMU_WASM_OUT  = $${QEMU_WASM_HOME}/out
    # 1) objets reels du link meson M1, DANS L'ORDRE DE LA REGLE c_LINKER
    #    (liste regenerable par recettes-wasm/gen-objs-list.sh du depot
    #    qemu/ branche poc-b — 668 objets : la regle en compte 669, moins
    #    system_main.c.o dont le main() est replique par le backend).
    #    Lie en brut :
    #    les .init_array (registres de types QEMU, constructors de modules)
    #    ne s'activent que si l'objet fait partie du link final — un .a
    #    archive ne tire un objet que si un symbole est reference, ce qui
    #    casse les types QOM referencees uniquement par chaine ("parent").
    #    Copie canonique : a cote de l'arbre out (defaut), surcharge par env.
    QEMU_OBJS_LIST = $$(QEMU_OBJS_LIST)
    isEmpty(QEMU_OBJS_LIST): QEMU_OBJS_LIST = $${QEMU_WASM_HOME}/qemu-objs.txt
    QEMU_OBJ_FILES = $$cat($${QEMU_OBJS_LIST}, lines)
    for(o, QEMU_OBJ_FILES) {
        LIBS += $${QEMU_WASM_OUT}/$${o}
    }
    # 2) archives en fin, dans le groupe meson (replication du lien M1).
    LIBS += -Wl,--start-group
    LIBS += $${QEMU_WASM_OUT}/libqemuutil.a
    LIBS += $${QEMU_WASM_OUT}/subprojects/dtc/libfdt/libfdt.a
    LIBS += -L$${QEMU_WASM_HOME}/target/lib -lglib-2.0 -lffi -lpcre2-8 -lz
    LIBS += -Wl,--end-group

    # coroutine wasm de QEMU 10.1 : emscripten_fiber_swap exige l'instrumentation
    # async (replication des drapeaux meson M1 : -sASYNCIFY=1 + import ffi_call_js)
    QMAKE_LFLAGS += -sASYNCIFY=1
    QMAKE_LFLAGS += -sASYNCIFY_IMPORTS=ffi_call_js

    # knobs wasm mkspec (wasm.prf) : memoire figee / pool de threads Qt
    QT_WASM_FIXED_MEMORY      = 1
    QT_WASM_INITIAL_MEMORY    = 2560MB
    QT_WASM_PTHREAD_POOL_SIZE = 8

    QMAKE_LFLAGS += -sFORCE_FILESYSTEM
    QMAKE_LFLAGS += -sALLOW_TABLE_GROWTH

    # POC C : le espsimu.html regenere par la regle de link du kit ne passe pas
    # les arguments de l'URL a qtLoad — re-patch idempotent apres chaque link.
    QMAKE_POST_LINK += python3 $$PWD/patch-html-qargs.py $$OUT_PWD/espsimu.html
}

