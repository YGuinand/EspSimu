#ifndef FENETRETERMINAL_H
#define FENETRETERMINAL_H

#include <QFrame>
#include <QCloseEvent>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QTimer>
#include <QStringDecoder>
#include <QTextCharFormat>
#include <QColor>

#include "modele.h"

// Zone de texte du terminal : affiche ce que l'ESP32 envoie et capte le clavier
// pour le transmettre (pas d'echo local : l'affichage vient de l'ESP32).
class TerminalEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit TerminalEdit(QWidget *parent = 0);

signals:
    void caracteresSaisis(const QByteArray &octets);   // texte tape ou colle
    void entreeAppuyee();

protected:
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void insertFromMimeData(const QMimeData *source);
};

// Modes d'affichage des sequences d'echappement ANSI recues sur l'UART.
enum ModeAffichage {
    ModeBrut,        // texte brut : sequences ANSI filtrees, pas d'interpretation
    ModeInterprete   // sequences ANSI interpretees (couleurs, gras, ...)
};

// Terminal sur la liaison serie UART0 de l'ESP32
class FenetreTerminal : public QFrame
{
    Q_OBJECT

public:
    explicit FenetreTerminal(Modele &m, QWidget *parent = 0);

    static const int UART = 0;

private:
    Modele *modele;
    TerminalEdit *edit;
    QComboBox *finDeLigne;
    QComboBox *choixMode;   // ModeBrut / ModeInterprete

    QByteArray recu;            // octets recus, en attente d'affichage
    QStringDecoder decodeur;    // UTF-8, conserve les caracteres coupes entre deux paquets
    QTimer *timer;
    int etatEchappement;        // analyse des sequences ANSI (couleurs des logs ESP-IDF)
    QByteArray paramsAnsi;      // paramètres de la sequence CSI en cours
    QByteArray tamponTexte;     // texte utile accumulate, attendre une sequence ou la fin
    QTextCharFormat formatCourant; // format (couleur, gras) applique au texte a afficher
    ModeAffichage m_modeAffichage;

    void flushTexte();          // decode et affiche tamponTexte avec formatCourant
    void appliquerSgr(const QByteArray &params);  // interprete les parametres SGR
    void afficher(const QString &texte);
    void setMode(int index);

private slots:
    void octetRecu(int uartId, char c);
    void viderRecu();
    void envoyer(const QByteArray &octets);
    void envoyerEntree();
    void effacer();

signals :
    void estCache(bool);

protected :
    virtual void closeEvent (QCloseEvent* event) {  event->ignore();  if (parentWidget()) parentWidget()->hide(); else hide(); emit estCache(true); }
};

#endif // FENETRETERMINAL_H
