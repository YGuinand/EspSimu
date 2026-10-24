#ifndef OSCILLO_H
#define OSCILLO_H

#include <functional>

#include <QChartView>
#include <QLineSeries>
#include <QPointF>
#include <QValueAxis>

#define NB_PTS  250

// Oscilloscope du signal PWM de commande d'un hacheur (sortie LEDC de l'ESP32).
// Le signal est reconstitue a partir du rapport cyclique et de la frequence lus dans QEMU,
// avec un declenchement sur front montant.
class Oscillo
{
private :
    QChart *figure;
    QLineSeries *courbe;
    QValueAxis *axisX, *axisY;

    // Renvoie le rapport cyclique (0..1) et la frequence (Hz) du signal observe
    std::function<bool(double &, double &)> source;

    int dt;                 // base de temps, en periodes d'une horloge de 48 MHz
    double xmem[NB_PTS];    // abscisses affichees (dans l'unite choisie)
    double unite;           // facteur secondes -> unite d'affichage
    int trigTemps;          // nombre d'echantillons avant le declenchement

    double tension(int i, double rapport, double frequence) const;

public:
    explicit Oscillo(QChartView *qp);

    void setSource(std::function<bool(double &, double &)> f) {  source = f;  }
    void setBaseTemps(int d);
    void rafraichir();
};

#endif // OSCILLO_H
