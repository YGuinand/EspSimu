#!/bin/sh
# Assemble le zip de livraison wasm depuis un build.
#
# Usage : livraison-wasm/make-dist.sh [rep_build_wasm]
#         (defaut : ../build/Wasm relatif a ce script)
#
# Produit : build/espsimu-wasm.zip  (non versionne) + le staging verifiable
# dans build/espsimu-wasm-dist/. Le sha256 du wasm est relit dans
# 3rdparty/qemu/VERSION (traceabilite) et injecte dans le README.

set -eu

SRC=${1:-"$(dirname "$0")/../build/Wasm"}
RACINE=$(cd "$(dirname "$0")/.." && pwd)
STAGE="$RACINE/build/espsimu-wasm-dist"
ZIP="$RACINE/build/espsimu-wasm.zip"

[ -f "$SRC/espsimu.wasm" ] || { echo "espsimu.wasm introuvable dans $SRC" >&2; exit 1; }

# Traceabilite : le sha256 du livrable de reference (section wasm du VERSION).
SH256=$(grep -oE '[0-9a-f]{64}' "$RACINE/3rdparty/qemu/VERSION" | head -1)
WASM_SH=$(sha256sum "$SRC/espsimu.wasm" | cut -d' ' -f1)
if [ "$SH256" != "$WASM_SH" ]; then
    echo "ATTENTION : sha256 du build ($WASM_SH) != VERSION ($SH256)." >&2
    echo "Le bundle a ete reconstruit sans reecrire la section wasm du VERSION." >&2
    exit 1
fi

TAILLE=$(stat -c '%s' "$SRC/espsimu.wasm")

rm -rf "$STAGE"
mkdir -p "$STAGE/licences"
cp "$SRC/espsimu.html" "$SRC/espsimu.js" "$SRC/espsimu.wasm" \
   "$SRC/qtloader.js" "$SRC/qtlogo.svg" "$STAGE/"
cp "$(dirname "$0")/serve-wasm.py" "$STAGE/"
cp "$(dirname "$0")/.htaccess" "$STAGE/"
sed -e "s/@SH256@/$SH256/" -e "s/@TAILLE@/$TAILLE/" \
    "$(dirname "$0")/README-LIVRAISON.md" > "$STAGE/README-LIVRAISON.md"
cp "$(dirname "$0")"/licences/* "$STAGE/licences/"

rm -f "$ZIP"
(cd "$STAGE" && zip -qr "$ZIP" .)
echo "Livrable : $ZIP"
sha256sum "$ZIP"
