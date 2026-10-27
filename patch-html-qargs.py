#!/usr/bin/env python3
# POC C Qt-wasm : le fichier espsimu.html produit par la regle de link du kit
# Qt ne passe pas les arguments de l'URL a qtLoad (Module.arguments). Ce
# re-patch idempotent, appele par QMAKE_POST_LINK apres chaque link, insere
# la lecture de ?qargs=... (une option par ligne, meme format que le desktop).
# Sans effet si le patch est deja present (regenerations repetees).
import sys

# Texte cible, base a l'identique sur le patch manuel qui a servi des jalons
# M2->M4 ; \\n est bien la sequence de deux caracteres backslash+n dans le
# produit final (echappement javascript, pas un saut de ligne).
ANCRE = """                const instance = await qtLoad({
                    qt: {"""
ARGS = r"""                const instance = await qtLoad({
                    // POC C : arguments recus via l'URL (?qargs=...\n...)
                    arguments: (new URLSearchParams(window.location.search).get('qargs') || '').split('\n').filter(a => a !== ''),
                    qt: {"""

def main(argv):
    if len(argv) != 2:
        sys.stderr.write("usage: patch-html-qargs.py <espsimu.html>\n")
        return 2
    chemin = argv[1]
    with open(chemin, "r", encoding="utf-8") as f:
        texte = f.read()
    if ARGS in texte:
        print(f"patch-html-qargs: deja patche {chemin}")
        return 0
    if ANCRE not in texte:
        sys.stderr.write(f"patch-html-qargs: ancre introuvable dans {chemin}\n")
        return 1
    with open(chemin, "w", encoding="utf-8") as f:
        f.write(texte.replace(ANCRE, ARGS, 1))
    print(f"patch-html-qargs: patche {chemin}")
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv))
