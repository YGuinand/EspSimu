#include "mainwindow.h"
#include <QApplication>

#include <QFile>
#include <QDebug>
#include <QTranslator>
#include <QtPlugin>

// Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);

QDebug *dbg;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/images/micropro.png"));
/*
    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
            QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    a.installTranslator(&qtTranslator);
*/
/*
    QTranslator qtTranslator;
    if (qtTranslator.load("qt_en")) {
        a.installTranslator(&qtTranslator);
    }
*/
    QFile f("./sortie.txt");
    f.open(QIODevice::WriteOnly);
    dbg = new QDebug(&f);

    MainWindow w;

    w.show();

    int ret;

    ret = a.exec();

    delete dbg;
    f.close();

    return ret;
}
