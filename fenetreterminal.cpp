#include "fenetreterminal.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QKeyEvent>
#include <QMimeData>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QFontDatabase>
#include <QFont>
#include <QList>
#include <QClipboard>
#include <QGuiApplication>

// ============================================================
// TerminalEdit
// ============================================================

TerminalEdit::TerminalEdit(QWidget *parent) : QPlainTextEdit(parent)
{
    setReadOnly(true);   // le texte vient uniquement de l'ESP32
    setUndoRedoEnabled(false);
    setLineWrapMode(QPlainTextEdit::WidgetWidth);
    setMaximumBlockCount(5000);
    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    setCursorWidth(0);
    setFocusPolicy(Qt::StrongFocus);
}

void TerminalEdit::keyPressEvent(QKeyEvent *event)
{
    // Copie / selection : comportement habituel de la zone de texte
    if (event->matches(QKeySequence::Copy) && textCursor().hasSelection()) {
        copy();
        return;
    }
    if (event->matches(QKeySequence::SelectAll)) {
        selectAll();
        return;
    }
    if (event->matches(QKeySequence::Paste)) {
        const QString texte = QGuiApplication::clipboard()->text();
        if (!texte.isEmpty()) emit caracteresSaisis(texte.toUtf8());
        return;
    }

    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        emit entreeAppuyee();
        return;
    case Qt::Key_Backspace:
        emit caracteresSaisis(QByteArray(1, '\b'));
        return;
    case Qt::Key_Tab:
        emit caracteresSaisis(QByteArray(1, '\t'));
        return;
    case Qt::Key_Escape:
        emit caracteresSaisis(QByteArray(1, '\x1b'));
        return;
    default:
        break;
    }

    // Ctrl+lettre : code de controle (Ctrl+C = 0x03, ...)
    if ((event->modifiers() & Qt::ControlModifier) && event->key() >= Qt::Key_A && event->key() <= Qt::Key_Z) {
        emit caracteresSaisis(QByteArray(1, char(event->key() - Qt::Key_A + 1)));
        return;
    }

    const QString texte = event->text();
    if (!texte.isEmpty() && !texte.at(0).isNonCharacter() && texte.at(0).unicode() >= 0x20) {
        emit caracteresSaisis(texte.toUtf8());
        return;
    }

    // Navigation (defilement, PageUp...) : traitement standard
    QPlainTextEdit::keyPressEvent(event);
}

void TerminalEdit::insertFromMimeData(const QMimeData *source)
{
    if (source->hasText()) emit caracteresSaisis(source->text().toUtf8());
}

// ============================================================
// FenetreTerminal
// ============================================================

FenetreTerminal::FenetreTerminal(Modele &m, QWidget *parent) :
    QFrame(parent),
    modele(&m),
    decodeur(QStringDecoder::Utf8),
    etatEchappement(0),
    tamponTexte(),
    formatCourant(),
    m_modeAffichage(ModeBrut)
{
    // Format de base : texte noir sur fond blanc (couleurs par defaut du terminal).
    // On définit explicitement un fond blanc pour eviter qu'un format "vide"
    // (QTextCharFormat() a un brush de fond noir invalide) ne produise du
    // texte noir sur fond noir.
    formatCourant.setForeground(Qt::black);
    formatCourant.setBackground(Qt::white);

    setWindowTitle(tr("Terminal"));
    setWindowIcon(QIcon(":/images/icone_terminal.png"));
    resize(640, 360);

    edit = new TerminalEdit(this);
    edit->setToolTip(tr("Liaison série UART0 : les caractères tapés ici sont envoyés à l'ESP32"));

    finDeLigne = new QComboBox(this);
    finDeLigne->addItem(tr("LF (\\n)"), QByteArray("\n"));
    finDeLigne->addItem(tr("CR (\\r)"), QByteArray("\r"));
    finDeLigne->addItem(tr("CR+LF (\\r\\n)"), QByteArray("\r\n"));
    finDeLigne->addItem(tr("Aucune"), QByteArray());
    finDeLigne->setToolTip(tr("Caractères envoyés quand on appuie sur Entrée"));

    choixMode = new QComboBox(this);
    choixMode->addItem(tr("Texte brut"), ModeBrut);
    choixMode->addItem(tr("Séquences ANSI interprétées"), ModeInterprete);
    choixMode->setToolTip(tr("Mode d'affichage des séquences d'échappement reçues"));

    QPushButton *boutonEffacer = new QPushButton(tr("Effacer"), this);

    QHBoxLayout *barre = new QHBoxLayout;
    barre->addWidget(new QLabel(tr("Entrée envoie :"), this));
    barre->addWidget(finDeLigne);
    barre->addWidget(choixMode);
    barre->addStretch();
    barre->addWidget(boutonEffacer);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(barre);
    layout->addWidget(edit);

    timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(30);   // regroupe les octets recus pour limiter les rafraichissements
    connect(timer, &QTimer::timeout, this, &FenetreTerminal::viderRecu);

    connect(modele->getEsp32(), &Esp32Backend::uartTx, this, &FenetreTerminal::octetRecu);
    connect(edit, &TerminalEdit::caracteresSaisis, this, &FenetreTerminal::envoyer);
    connect(edit, &TerminalEdit::entreeAppuyee, this, &FenetreTerminal::envoyerEntree);
    connect(boutonEffacer, &QPushButton::clicked, this, &FenetreTerminal::effacer);
    connect(choixMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FenetreTerminal::setMode);
}

void FenetreTerminal::octetRecu(int uartId, char c)
{
    if (uartId != UART) return;
    recu.append(c);
    if (!timer->isActive()) timer->start();
}

void FenetreTerminal::setMode(int index)
{
    m_modeAffichage = static_cast<ModeAffichage>(choixMode->itemData(index).toInt());
}

void FenetreTerminal::flushTexte()
{
    if (tamponTexte.isEmpty()) return;

    QString texte = decodeur(tamponTexte);
    tamponTexte.clear();

    QString suite;
    for (const QChar ch : texte) {
        if (ch == QChar('\b')) {
            if (!suite.isEmpty()) {
                suite.chop(1);
            } else {
                QTextCursor curseur(edit->document()->lastBlock());
                curseur.movePosition(QTextCursor::EndOfBlock);
                if (curseur.positionInBlock() > 0) curseur.deletePreviousChar();
            }
        } else {
            suite.append(ch);
        }
    }
    if (!suite.isEmpty()) afficher(suite);
}

void FenetreTerminal::appliquerSgr(const QByteArray &params)
{
    if (params.isEmpty()) {
        // SGR vide = reset
        formatCourant = QTextCharFormat();
        return;
    }

    QList<QByteArray> codes = params.split(';');
    bool bold = formatCourant.fontWeight() == QFont::Bold;
    QColor foreground = formatCourant.foreground().color();
    QColor background = formatCourant.background().color();

    for (const QByteArray &code : codes) {
        bool ok;
        int n = code.toInt(&ok);
        if (!ok) continue;

        switch (n) {
        case 0:   // reset
            formatCourant = QTextCharFormat();
            formatCourant.setForeground(Qt::black);
            formatCourant.setBackground(Qt::white);
            bold = false;
            foreground = Qt::black;
            background = Qt::white;
            break;
        case 1:   bold = true; break;           // gras
        case 22:  bold = false; break;          // pas gras
        case 30:  foreground = Qt::black; break;
        case 31:  foreground = Qt::red; break;
        case 32:  foreground = Qt::green; break;
        case 33:  foreground = Qt::yellow; break;
        case 34:  foreground = Qt::blue; break;
        case 35:  foreground = Qt::magenta; break;
        case 36:  foreground = Qt::cyan; break;
        case 37:  foreground = Qt::white; break;
        case 39:  foreground = Qt::black; break; // couleur par defaut
        case 40:  background = Qt::black; break;
        case 41:  background = Qt::red; break;
        case 42:  background = Qt::green; break;
        case 43:  background = Qt::yellow; break;
        case 44:  background = Qt::blue; break;
        case 45:  background = Qt::magenta; break;
        case 46:  background = Qt::cyan; break;
        case 47:  background = Qt::white; break;
        case 49:  background = Qt::white; break; // fond par defaut
        default:  break;
        }
    }

    formatCourant.setForeground(foreground);
    formatCourant.setBackground(background);
    QFont f = formatCourant.font();
    f.setBold(bold);
    formatCourant.setFont(f);
}

void FenetreTerminal::viderRecu()
{
    // Traite les octets recus un par un pour pouvoir changer de format
    // au sein d'un meme paquet (sequences ANSI entrecalees avec du texte).
    for (char c : recu) {
        unsigned char u = (unsigned char)c;
        if (etatEchappement == 1) {            // apres ESC
            if (c == '[') {
                etatEchappement = 2;
                paramsAnsi.clear();
            } else {
                etatEchappement = 0;
            }
            continue;
        }
        if (etatEchappement == 2) {            // dans une sequence CSI
            if ((u >= '0' && u <= '9') || u == ';') {
                paramsAnsi.append(c);
                continue;
            }
            if (u >= 0x40 && u <= 0x7E) {      // octet final de la sequence CSI
                if (m_modeAffichage == ModeInterprete && c == 'm') {
                    flushTexte();               // affiche le texte precede avec l'ancien format
                    appliquerSgr(paramsAnsi);   // change le format pour la suite
                }
                etatEchappement = 0;
                continue;
            }
            // Caractere invalide dans une CSI : on abandonne la sequence
            etatEchappement = 0;
            continue;
        }
        if (c == '\x1b') { etatEchappement = 1; continue; }
        if (c == '\r') continue;               // fin de ligne = LF uniquement
        tamponTexte.append(c);
    }
    recu.clear();
    flushTexte();
}

void FenetreTerminal::afficher(const QString &texte)
{
    QScrollBar *barre = edit->verticalScrollBar();
    bool enBas = barre->value() >= barre->maximum() - 2;

    QTextCursor curseur(edit->document());
    curseur.movePosition(QTextCursor::End);
    curseur.setCharFormat(formatCourant);
    curseur.insertText(texte);

    if (enBas) barre->setValue(barre->maximum());
}

void FenetreTerminal::envoyer(const QByteArray &octets)
{
    if (octets.isEmpty()) return;
    if (!modele->isEsp32Running()) return;   // ESP32 eteint : rien n'est recu
    modele->getEsp32()->sendUart(UART, octets);
}

void FenetreTerminal::envoyerEntree()
{
    envoyer(finDeLigne->currentData().toByteArray());
}

void FenetreTerminal::effacer()
{
    edit->clear();
    formatCourant = QTextCharFormat();
    formatCourant.setForeground(Qt::black);
    formatCourant.setBackground(Qt::white);
}
