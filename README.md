# EspSimu

Simulateur de carte ESP32 en C++/Qt 6 : périphériques (LED, boutons, potentiomètre,
LCD, oscilloscope), console série et backend de simulation QEMU (portage wasm,
 liaison statique). Anciennement PicSimu (microcontrôleurs PIC).

- **Application desktop** : `espsimu.pro` (qmake).
- **Version navigateur (WebAssembly)** : livrable autonome dans
  [`livraison-wasm/`](livraison-wasm/README-LIVRAISON.md) — serveur local,
  limites et navigateurs validés.
- **Firmwares de test** : dépôt `testEspSimu` (PlatformIO).
- **Licence** : MIT (voir [LICENSE](LICENSE)) ; QEMU GPL-2.0 et Qt LGPL-3.0
  embarqués — détails dans `livraison-wasm/licences/`.
