#!/bin/sh

# Assemble le paquet Windows deployable pour EspSimu :
#   espsimu.exe
# + toutes les dependances DLL recursives du sys-root MinGW (hors systeme Windows)
# + plugins Qt (platforms, styles, imageformats, iconengines, netw. info, tls)
# + pc-bios (ROM ESP32 : esp32-v3-rom*.bin)
# + libqemu-xtensa.dll / libqemu-riscv32.dll (build par qemu/build_libqemu-esp32-windows.sh)
#
# Usage :
#   ./paquet-windows.sh [RepertoireDeSortie]          # par defaut /tmp/espsimu-windows
#   ESPSIMU_EXE=... ./paquet-windows.sh nom-paquet   # exe alternatif
#
# Prerequis :
#   - x86_64-w64-mingw32-objdump (binutils croises)
#   - sys-root MinGW avec Qt : /usr/x86_64-w64-mingw32/sys-root/mingw
#   - libqemu-*.dll deja construites (elles sont copiees telles quelles)

set -e

SYSROOT="/usr/x86_64-w64-mingw32/sys-root/mingw"
OBJDUMP="${CROSS:-x86_64-w64-mingw32-}objdump"
PKG="${1:-/tmp/espsimu-windows}"
SIMU_HOME="$(cd "$(dirname "$0")" && pwd)"

# exe : priorite a ESPSIMU_EXE, sinon celui du shadow-build crois� habituel
EXE="${ESPSIMU_EXE:-$SIMU_HOME/build/Windows_Cross/release/espsimu.exe}"
[ -f "$EXE" ] || { echo "ERREUR: $EXE introuvable (construisez d'abord l'exe croise, cf. AGENTS.md)"; exit 1; }
echo "exe: $EXE"

rm -rf "$PKG"
mkdir -p "$PKG/3rdparty/qemu/lib"

# ---------------------------------------------------------------- exe + qemu
cp "$EXE" "$PKG/"

for d in "$SIMU_HOME/3rdparty/qemu/lib/libqemu-xtensa.dll" \
         "$SIMU_HOME/3rdparty/qemu/lib/libqemu-riscv32.dll"; do
  if [ -f "$d" ]; then cp "$d" "$PKG/3rdparty/qemu/lib/";
  else echo "AVERTISSEMENT: $d absente (cf. qemu/build_libqemu-esp32-windows.sh)"; fi
done

# ---------------------------------------------------------------- pc-bios
# ROMs ESP32 : soit une copie locale EspSimu/pc-bios, soit le fork qemu voisin
mkdir -p "$PKG/pc-bios"
for dir in "$SIMU_HOME/pc-bios" \
           "$SIMU_HOME/build/Desktop_Debug/pc-bios" \
           "${QEMU_HOME:-$SIMU_HOME/../qemu}/pc-bios"; do
  if ls "$dir"/esp32-v3-rom*.bin >/dev/null 2>&1; then
    cp "$dir"/esp32-v3-rom*.bin "$PKG/pc-bios/"
    echo "pc-bios <- $dir"
    break
  fi
done
[ -f "$PKG/pc-bios/esp32-v3-rom.bin" ] || echo "AVERTISSEMENT: pc-bios introuvable"

# ---------------------------------------------------------------- plugins Qt
QT_PLUGINS="$SYSROOT/lib/qt6/plugins"
for p in platforms styles imageformats iconengines networkinformation tls; do
  [ -d "$QT_PLUGINS/$p" ] && mkdir -p "$PKG/$p" && cp "$QT_PLUGINS/$p"/*.dll "$PKG/$p/"
done

# --------------------------------- dependances DLL recursives (hors systeme)
# DLL fournies par Windows lui-meme : jamais copiees dans le paquet.
systeme() {
  case "$(echo "$1" | tr 'A-Z' 'a-z')" in
    kernel32.dll|user32.dll|gdi32.dll|shell32.dll|msvcrt.dll|advapi32.dll|ole32.dll|oleaut32.dll|uuid.dll|comdlg32.dll|winspool.drv|ws2_32.dll|iphlpapi.dll|winmm.dll|crypt32.dll|secur32.dll|netapi32.dll|userenv.dll|uxtheme.dll|imm32.dll|shlwapi.dll|shcore.dll|dwmapi.dll|propsys.dll|ntdll.dll|rpcrt4.dll|setupapi.dll|cfgmgr32.dll|powrprof.dll|d3d11.dll|d3d12.dll|dxgi.dll|opengl32.dll|glu32.dll|wtsapi32.dll|samcli.dll|version.dll|win32u.dll|gdi32full.dll|msimg32.dll|kernelbase.dll|dnsapi.dll|winhttp.dll|mpr.dll|authz.dll|dwrite.dll|d2d1.dll|d3d9.dll|bcrypt.dll|ncrypt.dll|sspicli.dll|cryptbase.dll|bcryptprimitives.dll|api-ms-win-*) return 0 ;;
    *) return 1 ;;
  esac
}

copier_deps() {
  local ajoute=0
  # Tout PE du paquet (exe a la racine, DLL a la racine et dans les sous-dossiers)
  for f in $(find "$PKG" -maxdepth 4 \( -name '*.exe' -o -name '*.dll' \)); do
    for dll in $($OBJDUMP -p "$f" 2>/dev/null | awk '/DLL Name:/{print $3}'); do

      systeme "$dll" && continue
      if [ ! -f "$PKG/$dll" ]; then
        if [ -f "$SYSROOT/bin/$dll" ]; then
          cp "$SYSROOT/bin/$dll" "$PKG/$dll"
          echo "  + $dll  <- $SYSROOT/bin/"
          ajoute=1
        else
          echo "ERREUR: $dll (requis par $(basename "$f")) introuvable dans $SYSROOT/bin/"
          exit 1
        fi
      fi
    done
  done
  # Fixpoint : les DLL ajoutees ont chacune leurs propres dependances
  if [ $ajoute -eq 1 ]; then copier_deps; fi
  return 0
}

echo "=== resolution recursive des dependances ==="
copier_deps

# ------------------------------------------------------------------ controle
echo ""
echo "=== verification des architectures (tout doit etre PE32+ x86-64) ==="
ok=1
for f in $(find "$PKG" \( -name '*.exe' -o -name '*.dll' \)); do
  if ! file "$f" | grep -q "PE32+"; then
    echo "INCOMPATIBLE: $f"; file "$f"; ok=0
  fi
done
[ $ok -eq 1 ] && echo "OK"

echo ""
echo "=== Paquet pret : $PKG ==="
find "$PKG" -type f | sort | sed "s|^$PKG/|  |"
du -sh "$PKG" | awk '{print "  total : "$1}'
