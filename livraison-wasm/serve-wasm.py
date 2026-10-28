#!/usr/bin/env python3
# Serveur de livraison pour EspSimu wasm.
#
# Le simulateur webassembly est multi-thread (SharedArrayBuffer) : le
# navigateur n'active ce mode que si la page est servie avec les en-tetes
# d'isolement cross-origin COOP/COEP. Un hebergeur statique classique
# (GitHub Pages, p. ex.) ne permet pas de poser ces en-tetes : ce serveur
# les ajoute, ainsi qu'un anti-cache evitant de mixer un ancien et un
# nouveau bundle au rechargement de la page.
#
# Usage :  python3 serve-wasm.py [port]     (defaut : 8137, sert ce dossier)
# Puis ouvrir http://127.0.0.1:8137/espsimu.html

import functools
import http.server
import os
import sys

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8137
RACINE = os.path.dirname(os.path.abspath(__file__))


class requetes(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=RACINE, **kwargs)

    def end_headers(self):
        # Isolement cross-origin requis par SharedArrayBuffer (threads wasm).
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        # Evite le melange ancien/nouveau bundle dans le cache du navigateur.
        self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
        super().end_headers()

    def log_message(self, *args):
        pass


if __name__ == "__main__":
    serveur = http.server.ThreadingHTTPServer(("127.0.0.1", PORT), requetes)
    print(f"EspSimu wasm servi depuis {RACINE}")
    print(f"Ouvrir : http://127.0.0.1:{PORT}/espsimu.html")
    try:
        serveur.serve_forever()
    except KeyboardInterrupt:
        pass
