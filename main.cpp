#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/images/micropro.png"));

    MainWindow w;
    w.show();

    // Mode diagnostic : demarrage automatique de l'ESP32 sur le firmware donne
    // (--diag-autostart <fichier.bin>), pour reproduire le lag post-reset
    // hors ecran. Combine a ESPSIMU_DIAG=1 du backend.
    const QStringList args = a.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == "--diag-autostart" && i + 1 < args.size()) {
            w.demarrageDiagnostic(args[++i]);
            break;
        }
    }

    return a.exec();
}
