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

Initialization file: at startup the app reads `espsimu.ini` **next to the executable** (`parametres.*`, file is created on first save; gitignored at repo root). It stores the last loaded ESP32 firmware, the last gamelle `.gml` (fenetre Gamel) and the last piste `.pis` (fenetre Piste); each is reloaded at the next launch if the file still exists (loaded, not started — Power turns the ESP32 on).

Window-close semantics (2026-10-24): closing the MainWindow ends the application. `MainWindow::closeEvent` runs in two passes (pass 1 ignores + shows a modal progress, pass 2 accepts); the delayed pass calls `Modele::shutdownEsp32()` — full QEMU main-loop stop (shutdown request exits `qemu_main_loop` immediately, `qemu_cleanup` ~10 ms, then thread join) — followed by an explicit `QApplication::quit()`. Do not rely on `quitOnLastWindowClosed`: the progress dialog would otherwise remain the last visible window and the app would hang with the QEMU thread still attached (historical "RIP=0" teardown crashes came from forced kills of that hung state; the QEMU lib also installs its own SIGTERM handler). The Power button keeps pause semantics (`stopEsp32`, thread alive). The whole chain is instrumented with `EspDiag::mark` (`ESPSIMU_DIAG=1`).

## Translations

`espsimu_en.ts` / `espsimu_it.ts` (Qt Linguist). `.qm` files are produced by `lrelease` and gitignored; no codegen step is wired into the build.

## Repo layout

- Flat source tree: one `.pro`, ~20 `.cpp/.h` pairs, 6 `.ui` forms, `ressources.qrc` (embeds `images/` and `piste001.pis`, plus `boot_app0.bin`: the 8 KiB Arduino OTA-app placeholder injected at `OFFSET_BOOT_APP0` by `findBootApp0` so a plain `firmware.bin` loads on machines without a local PlatformIO install), `3rdparty/qemu/lib/libqemu-xtensa.so`.
- Entry point: `main.cpp` → `MainWindow` (`mainwindow.*`). No library or test subprojects.
- Key pieces: `esp32backend.*` (QEMU shared lib wrapper, GPIO/ADC/LEDC access), `modele.*` (robot model: contacts, IR sensors, motors, track), `grovelcdrgb.*` + `lcdscreen.*` (I2C Grove LCD), `gamel.*`/`fenetre*.*` (windows), `oscillo.*` (PWM scope built from LEDC duty/frequency). Board pin mapping (J2 connector, IOxx) lives in `Esp32Pins` (`esp32backend.h`); `VCC_CARTE` = 3.3 V in `connexion.h`.
- `picsimlab` in `esp32backend.cpp` (`qemu_picsimlab_*`, machine `esp32-picsimlab`) is the external QEMU fork API: do not rename it. Sole approved exception (2026-10-24): `qemu_picsimlab_set_apin` → `qemu_picsimlab_set_apin_mv` when analog injection switched from raw counts to millivolts (the app-side wrapper is `Esp32Backend::setAnalogPinMV`; the old counts symbol was dropped on purpose as an ABI guard — an older lib makes the ESP32 part fail to load).
- QEMU lib traceability: `3rdparty/qemu/VERSION` maps each delivered lib (`libqemu-*.so/.dll`, gitignored local artifacts) to the qemu fork commit that built it. After every QEMU rebuild: copy the lib into `3rdparty/qemu/lib/` as usual, then update `VERSION` (full `git rev-parse HEAD` captured *at build time* + copy timestamp) and tag the qemu repo (`libqemu-esp32-<date>`); validate with `ESPSIMU_DIAG=1 ./espsimu --diag-autostart <bin>` before shipping the new lib.
- Since lib 2026-10-24-2 (tag `libqemu-esp32-2026-10-24-2`) the ESP32 ADC is a modeled SAR, not a pass-through: `hw/misc/esp32_sens.c` inverts the IDF5 line-fitting factory curve for the burned Vref, so `analogReadMilliVolts()` reads back the injected millivolt value (dead zone 142/128 mV at atten 11 dB, identity mid-range, saturation plateau ≈3145 mV), honor `analogReadResolution` (9–12 bits) via SENS width registers and the per-channel attenuation registers (re-read at each conversion), and complete in ~5 µs at the IDF5 default config (RTC 200 ksp/s datasheet bound). The lib from tag `libqemu-esp32-2026-10-24` additionally burns a factory `ADC_VREF` eFuse field (BLK0 w4 bits [12:8], 1114 mV) at read-op when the efuse image leaves it blank — prerequisite of the line-fitting scheme: without it `adc_cali_create_scheme_line_fitting` fails (`default vref didn't set`) and `analogReadMilliVolts()` returns 0. Older libs (and any `espsimu.exe` built before the rename) have neither the eFuse burn nor the `_mv` API — on Windows the DLLs and the crossed exe are rebuilt together (`qemu/build_libqemu-esp32-windows.sh`, then the `build/Windows_Cross` exe, both from the same source tag; an older exe keeps calling the counts symbol and fails to load the new DLL, "Symbole manquant"); the DLLs and exe were rebuilt 2026-10-24, `paquet-windows.sh` assembles the final bundle (see `3rdparty/qemu/VERSION`).
- Windows cross support (2026): `paquet-windows.sh` assembles the deployable package into `/tmp/espsimu-windows` (default): it does **not** build anything — build the exe first (`build/Windows_Cross/`, `x86_64-w64-mingw32-qmake-qt6` + `make`, manual), then the script uses it (`ESPSIMU_EXE` overrides the default path), copies QEMU DLLs from `../qemu/build_libqemu-esp32-windows.sh` output (`3rdparty/qemu/lib/*.dll`), `pc-bios/` ESP32 ROMs, Qt plugins (`platforms/qwindows.dll` mandatory), and resolves MinGW/Qt DLL deps recursively from the sys-root via `x86_64-w64-mingw32-objdump` (Windows system DLLs are never copied; arch is verified PE32+ x86-64).
- UI strings and comments are in French; identifiers are mixed English/French. Match surrounding style — no enforced formatter.
