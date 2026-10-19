# EspSimu — agent notes

Single-project C++/Qt 6 desktop app (simulator of a robot board driven by an ESP32, run in QEMU). Build system: qmake (`espsimu.pro`). The former PIC microcontroller emulation has been removed. There are **no tests, no CI, and no linter/formatter configured** — do not invent or run any.

## Build

Toolchain verified on this machine: Qt 6.11.2, qmake 3.1, gcc 16, C++20.

Use a **shadow build directory** (never run qmake in the repo root):

    mkdir -p build/Desktop_Debug
    cd build/Desktop_Debug
    qmake ../../espsimu.pro
    make

Release variant: `build/Desktop_Release` with the same two commands. The .pro has no debug/release switch; QtCreator selects it via the kit.

Generated files are gitignored and must not be committed: `moc_*`, `ui_*.h`, `qrc_*.cpp`, `Makefile*`, `*.o`, `*.qm`, `.qmake.stash`, `*build-*`. The `build/` directory is local-only.

## Run

    cd build/Desktop_Debug
    ./espsimu

ESP32 backend quirk: `modele.cpp` loads `3rdparty/qemu/lib/libqemu-xtensa.so` relative to the executable's directory (`QCoreApplication::applicationDirPath()`). The binary in `build/Desktop_Debug/` cannot find it there — the app prints "echec chargement QEMU" and still runs, but ESP32 features are unavailable. To run with ESP32 support, place the binary next to the repo's `3rdparty/` directory so `<exeDir>/3rdparty/qemu/lib/libqemu-xtensa.so` resolves.

## Translations

`espsimu_en.ts` / `espsimu_it.ts` (Qt Linguist). `.qm` files are produced by `lrelease` and gitignored; no codegen step is wired into the build.

## Repo layout

- Flat source tree: one `.pro`, ~20 `.cpp/.h` pairs, 6 `.ui` forms, `ressources.qrc` (embeds `images/` and `piste001.pis`), `3rdparty/qemu/lib/libqemu-xtensa.so`.
- Entry point: `main.cpp` → `MainWindow` (`mainwindow.*`). No library or test subprojects.
- Key pieces: `esp32backend.*` (QEMU shared lib wrapper, GPIO/ADC/LEDC access), `modele.*` (robot model: contacts, IR sensors, motors, track), `grovelcdrgb.*` + `lcdscreen.*` (I2C Grove LCD), `gamel.*`/`fenetre*.*` (windows), `oscillo.*` (PWM scope built from LEDC duty/frequency). Board pin mapping (J2 connector, IOxx) lives in `Esp32Pins` (`esp32backend.h`); `VCC_CARTE` = 3.3 V in `connexion.h`.
- `picsimlab` in `esp32backend.cpp` (`qemu_picsimlab_*`, machine `esp32-picsimlab`) is the external QEMU fork API: do not rename it.
- UI strings and comments are in French; identifiers are mixed English/French. Match surrounding style — no enforced formatter.
- The working tree has uncommitted changes (ESP32/Grove LCD update, v2.00): check `git status` before assuming the tree is clean.
