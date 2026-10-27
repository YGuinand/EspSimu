#include "mainwindow.h"
#include <QApplication>
#ifdef __EMSCRIPTEN__
#include <cstdlib>
#endif

int main(int argc, char *argv[])
{
#ifdef __EMSCRIPTEN__
    // POC C M2 (wasm) : pas d'ENV navigateur exportable simplement, on force
    // le diagnostic EspDiag identiquement a ESPSIMU_DIAG=1 du desktop et la
    // longue phase PWM sans reset automatique (ESPSIMU_DIAG_NORESET=1).
    setenv("ESPSIMU_DIAG", "1", 1);          // no-op si deja present
    setenv("ESPSIMU_DIAG_NORESET", "1", 1);
#endif
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/images/micropro.png"));

    MainWindow w;
    w.show();

    // Mode diagnostic : demarrage automatique de l'ESP32 sur le firmware donne
    // (--diag-autostart <fichier.bin>), pour reproduire le lag post-reset
    // hors ecran. Combine a ESPSIMU_DIAG=1 du backend.
    // --gamelle <fichier.gml> pre-charge la gamelle (csv cablage/capteurs/
    // hacheurs) avant le firmware : reproduit le scenario complet du diagnostic.
    const QStringList args = a.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == "--diag-autostart" && i + 1 < args.size()) {
            QString fichierGamelle;
            // --gamelle peut preceder ou suivre --diag-autostart
            for (int j = 1; j < args.size() - 1; ++j) {
                if (args[j] == "--gamelle") fichierGamelle = args[j + 1];
            }
            w.demarrageDiagnostic(args[++i], fichierGamelle);
            break;
        }
    }

    return a.exec();
}
