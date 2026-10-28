# EspSimu — livraison WebAssembly

Simulateur de carte ESP32 embarqué dans le navigateur : interface Qt, backend
de simulation QEMU (portage wasm, lié statiquement). Ce dossier est le
livrable autonome — il se sert et s'exécute tel quel, sans installation.

## Contenu

| Fichier             | Rôle                                                  |
|---------------------|-------------------------------------------------------|
| `espsimu.html`      | page hôte (canvas de rendu, chargement du bundle)     |
| `espsimu.js`        | runtime Emscripten (preload, threads, MEMFS)          |
| `espsimu.wasm`      | application complète : Qt 6 + EspSimu + QEMU wasm     |
| `qtloader.js`       | chargement/démarrage de l'application Qt              |
| `qtlogo.svg`        | écran de chargement                                   |
| `serve-wasm.py`     | serveur local avec les en-têtes requis (COOP/COEP)    |
| `licences/`         | textes de licence et notices                          |

## Lancement

```sh
python3 serve-wasm.py          # port 8137 par défaut
```

Ouvrir http://127.0.0.1:8137/espsimu.html — cliquer **Ouvrir** pour charger un
firmware (`firmware_merged.bin`), puis **Power**. Le premier chargement
télécharge ~38 Mo et démarre la VM en ~9 s.

Les en-têtes `Cross-Origin-Opener-Policy` / `Cross-Origin-Embedder-Policy`
posés par le serveur sont obligatoires : le simulateur est multi-thread et le
navigateur ne fournit `SharedArrayBuffer` qu'aux pages ainsi isolées. Un
hébergeur statique sans en-têtes personnalisables (GitHub Pages, p. ex.)
n'est pas utilisable tel quel.

## Navigateurs

| Navigateur | État |
|------------|------|
| Firefox    | Validé (chaîne complète automatisée : upload, boot, pause/reprise, rechargement firmware) |
| Chromium / Chrome | Moteur validé (même chaîne complète) ; le sélecteur de fichiers natif (`showOpenFilePicker`) ne peut pas s'ouvrir en mode headless, l'usage réel dans une fenêtre est attendu conforme |
| Safari     | Non testé |

## Limites connues (wasm)

- **Mémoire** : ~2,56 Go réservés d'avance (`SharedArrayBuffer` figé) — un
  appareil justement doté peut refuser l'allocation.
- **Boot** : ~9 s (init QEMU complète dans le navigateur).
- **UART** : ~15 Ko/s vers le guest (débit applicatif, pas une limite du port
  série simulé).
- **Fichiers éphémères** : le système de fichiers du navigateur (MEMFS) vit le
  temps de la page. Recharger la page efface le firmware chargé — il faut le
  re-téléverser via **Ouvrir**. Le bouton **Recharger le fichier** rouvre le
  sélecteur de fichiers pour re-téléverser un firmware reconstruit (le
  navigateur ne peut pas relire votre disque sans dialogue).
- **Pas d'accès réseau/réel** : le guest vit dans le simulateur (pas de pont
  Wi-Fi/USB).

## Dépannage

- **Page blanche ou ancien bundle après une mise à jour** : forcer un
  rechargement complet (Ctrl+Shift+R). Le serveur envoie déjà un anti-cache.
- **L'application ne démarre pas** : vérifier dans la console du navigateur
  que `crossOriginIsolated` vaut `true` (sinon les en-têtes COOP/COEP sont
  absents — utiliser `serve-wasm.py`).
- **Diagnostic intégré** : ouvrir `espsimu.html?qargs=ESPSIMU_DIAG=1` pour
  tracer la boucle principale et l'UART sur la console.

## Traçabilité

- `espsimu.wasm` : @TAILLE@ octets — sha256 `@SH256@`.
- Version QEMU embarquée + historique : voir `3rdparty/qemu/VERSION` du dépôt
  EspSimu.
- Sources QEMU (GPL-2.0) : https://github.com/YGuinand/EspSimu_qemu,
  branche `poc-b`.
- Sources de l'application : dépôt EspSimu (recette de build qmake incluse).

## Licences

- **EspSimu** (cette application) : MIT — `licences/LICENSE-EspSimu-MIT.txt`.
- **QEMU** (portage wasm, lié statiquement) : GNU GPL v2 —
  `licences/LICENSE-QEMU-GPL-2.0.txt` ; sources via le dépôt public ci-dessus.
- **Qt 6** (liaison statique) : GNU LGPL v3 —
  `licences/LICENSE-Qt-LGPL-3.0.txt`. Conformément à la LGPL pour la liaison
  statique, le code source de l'application et sa recette de build (permettant
  de relinker l'application avec une version modifiée de Qt) sont disponibles
  dans le dépôt EspSimu.
