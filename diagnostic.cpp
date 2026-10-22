#include "diagnostic.h"

#include <QAtomicInt>
#include <QDebug>
#include <QElapsedTimer>
#include <QMutex>
#include <QString>
#include <QThread>
#include <QVector>
#include <climits>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#define DIAG_SYM(handle, name) (void *)GetProcAddress((HMODULE)(handle), (name))
#else
#include <dlfcn.h>
#define DIAG_SYM(handle, name) dlsym((handle), (name))
#endif

// ==================== Symboles QEMU ====================
// qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) : temps virtuel courant de la VM.
// qemu_clock_has_timers / qemu_clock_deadline_ns_all : etat des temporisateurs
// virtuels au moment ou le temps se fige.
// Valeurs fixees a la main (include/qemu/timer.h du fork QEMU) puisque ce
// module passe par dlsym.
typedef int64_t (*t_qemu_clock_get_ns)(int);
typedef bool    (*t_qemu_clock_has_timers)(int);
typedef int64_t (*t_qemu_clock_deadline_ns_all)(int, int);

#define QEMU_CLOCK_VIRTUAL   1
#define QEMU_TIMER_ATTR_ALL  0xffffffff

// Declaration anticipée : ecartRafales() peut declencher un dump.
namespace EspDiag { void dump(const QString &raison); }

namespace EspDiag {

namespace {

const int    PERIODE_ECH_MS = 20;               // periode d'echantillonnage du temps virtuel
const int    NB_ECH_MAX     = 10000;            // 200 s d'echantillons
const int    NB_RAF_MAX     = 512;              // rafales UART memorisees
const int    NB_RUPT_MAX    = 64;               // periodes figees memorisees
const int    NB_MARQ_MAX    = 64;               // marqueurs memorises
const qint64 NS_PAR_MS       = 1000000;
const qint64 SEUIL_FIGE_NS   = 2 * NS_PAR_MS;    // dv en dessous : temps virtuel fige
const qint64 SEUIL_RUPT_NS   = 100 * NS_PAR_MS;  // duree minimale d'une rupture loguee
const qint64 GAP_RAF_NS      = 5 * NS_PAR_MS;    // rafale UART : octets < 5 ms d'ecart
const qint64 SEUIL_ECAR_UART = 1300 * NS_PAR_MS; // ecart UART important : log + dump auto
const qint64 SEUIL_LIVRAISON = 150 * NS_PAR_MS;  // retard de livraison Qt sur l'emission
const double RATIO_LENT      = 0.5;              // cadence virtuelle/mur dite lente

struct Echantillon {                    // temps mur + temps virtuel (ns)
    qint64 w, v;
};

struct Rupture {                        // periode ou le temps virtuel est fige
    qint64 w0 = 0, w1 = 0, v0 = 0, v1 = 0;
    bool   ouverte = false;
    bool   timersActifs = false;
    qint64 echeanceNs;
    Rupture() : echeanceNs(LLONG_MIN) {}
};

struct Marque {                         // marqueur d'evenement
    qint64 w, v;
    const char *label;                  // litteraux uniquement
};

struct Rafale {                         // rafale d'octets UART (ecarts < 5 ms)
    int    id = 0;                      // n° d'ordre d'emission (source)
    int    uartId = 0;
    qint64 wDebS = 0, wFinS = 0;        // emission : temps mur
    qint64 vDebS = 0, vFinS = 0;        // emission : temps virtuel QEMU
    qint64 wFinQt = -1;                 // livraison approx. (appariement par n°)
    qint64 wDebQt = -1;                 // premiere arrivee cote Qt
    int    n = 0;                       // nombre d'octets
    char   apercu[25] = {};             // debut du contenu (pour identifier la ligne)
    int    nAper = 0;
};

struct Diagnostic {
    QElapsedTimer chrono;
    bool     actifMode = false;
    void    *lib = nullptr;
    QThread *echThread = nullptr;
    QAtomicInt echStop;
    int      compteurRafales = 0;

    QMutex mutex;

    t_qemu_clock_get_ns          p_tempsVirtuelNs = nullptr;
    t_qemu_clock_has_timers      p_timersActifs   = nullptr;
    t_qemu_clock_deadline_ns_all p_echeance       = nullptr;

    qint64 vOrigin = -1;                // temps virtuel de reference (1er ech. > 0)
    qint64 wT0 = -1;                    // temps mur de reference (init)
    qint64 wDernierReset = -1;          // marqueur "reset*" : origine des temps affiches

    QVector<Echantillon> echs;
    QList<Marque>        marques;
    QList<Rupture>       ruptures;
    Rupture              rupture;      // periode figee en cours
    bool                 lentFlag = false;

    QList<Rafale>        rafales;

    // rafale UART en cours cote source (thread QEMU)
    int    rafCouranteId = -1;
    int    rafUart = 0;
    qint64 rafWDeb = -1, rafWFin = -1, rafVDeb = -1, rafVFin = -1;
    int    rafN = 0;
    char   rafApercu[25] = {};
    int    rafAperN = 0;

    // rafale en cours cote livraison Qt (appariement par n° d'emission)
    int    qtCouranteId = -1;
    qint64 qtWDeb = -1, qtWFin = -1, qtWFinPrec = -1;

    bool   pauseVM = false;             // VM sur pause : le freeze du temps virtuel est attendu
};

Diagnostic d;

static void trace(const QString &m)
{
    // stderr direct : ne depend pas du routage des qDebug (journald, desactive en release...)
    fprintf(stderr, "EspDiag: %s\n", qUtf8Printable(m));
}

static double sec(qint64 ns)  { return ns / 1e9; }
static double msec(qint64 ns) { return ns / 1e6; }

// Temps virtuel QEMU, relatif a son origine detectee ; -1 si indisponible.
// A appeler mutex non tenu.
static qint64 tempsVirtuel()
{
    if (!d.p_tempsVirtuelNs) return -1;
    int64_t v = d.p_tempsVirtuelNs(QEMU_CLOCK_VIRTUAL);
    if (v <= 0) return -1;              // avant qemu_init / vm_start
    QMutexLocker l(&d.mutex);
    if (d.vOrigin < 0) d.vOrigin = v;
    return v - d.vOrigin;
}

// Affichage : t=+12.34s (dep. reset) ; a defaut, depuis l'init.
// Lecture simple des champs (wDernierReset, wT0) : appelable hors mutex.
static QString relTemps(qint64 w)
{
    qint64 base = (d.wDernierReset >= 0) ? d.wDernierReset : d.wT0;
    const char *ctx = (d.wDernierReset >= 0) ? "dep. reset" : "dep. init";
    return QString("t=%1s (%2)").arg(sec(w - base), 0, 'f', 2).arg(ctx);
}

// Interpretation d'un ecart entre deux emissions UART consecutives :
//   virtuel ≈ mur (~1.0 s pour le firmware de test) : cadence normale ;
//   mur >> virtuel : la VM tourne au ralenti sur la periode ;
//   virtuel ≈ >> attendu : le guest reste silencieux en avancant (boot,
//   polling, timer suspect). L'apercu des rafales permet de trancher.
// Hors mutex : ne lit que ses parametres et des champs simples.
static void ecartRafales(const Rafale &r, qint64 egoWall, qint64 egoVirt)
{
    if (egoWall < 0) return;
    // Fenetre de boot : ecarts lies au redemarrage du firmware, pas au phenomene
    bool fenetreBoot = (d.wDernierReset >= 0 && r.wDebS - d.wDernierReset < 5 * NS_PAR_MS);
    bool anormalLong = (egoWall > SEUIL_ECAR_UART) && !fenetreBoot;
    bool derapage = !fenetreBoot &&
                    (egoWall > 200 * NS_PAR_MS && egoVirt >= 0 &&
                     qAbs(egoWall - egoVirt) > SEUIL_LIVRAISON);
    if (!anormalLong && !derapage) return;

    QString conclusion;
    if (egoVirt < 0)
        conclusion = "temps virtuel indisponible";
    else if (qAbs(egoWall - egoVirt) <= SEUIL_LIVRAISON && egoWall > 1000 * NS_PAR_MS)
        conclusion = QString("guest silencieux %1 ms en temps virtuel == mur : CPU en cours ou en attente interne (boot/polling/timer)")
                         .arg(msec(egoWall), 0, 'f', 0);
    else
        conclusion = QString("VM lente sur la periode (temps virtuel n'a avance que de %1 ms sur %2)")
                         .arg(msec(egoVirt), 0, 'f', 0).arg(msec(egoWall), 0, 'f', 0);
    trace(QString("UART : ecart %1 rafale n%2 (uart%3) : source %4 ms, virtuel %5 ms -> %6")
              .arg(relTemps(r.wFinS)).arg(r.id).arg(r.uartId)
              .arg(msec(egoWall), 0, 'f', 0)
              .arg(egoVirt >= 0 ? QString::number(msec(egoVirt), 'f', 0) : "n/d")
              .arg(conclusion));
    if (anormalLong) dump("ecart UART apres reset");
}

// Mutex tenu : ferme la rafale source si le silence >= 5 ms la demande.
// Remplit r et les ecards (mur/virtuel) ; retourne true si fermeture.
static bool fermeRafaleSiSilencieuse(qint64 maintenant, Rafale &r,
                                     qint64 &egoWall, qint64 &egoVirt)
{
    egoWall = egoVirt = -1;
    if (d.rafCouranteId < 0 || maintenant - d.rafWFin < GAP_RAF_NS) return false;

    r.id = d.rafCouranteId; r.uartId = d.rafUart;
    r.wDebS = d.rafWDeb; r.wFinS = d.rafWFin;
    r.vDebS = d.rafVDeb; r.vFinS = d.rafVFin;
    r.n = d.rafN;
    r.nAper = d.rafAperN;
    memcpy(r.apercu, d.rafApercu, sizeof(r.apercu));

    if (!d.rafales.isEmpty()) {
        const Rafale &p = d.rafales.last();
        egoWall = r.wDebS - p.wFinS;
        egoVirt = r.vDebS - p.vFinS;
    }
    d.rafales.append(r);
    while (d.rafales.size() > NB_RAF_MAX) d.rafales.removeFirst();
    d.rafCouranteId = -1;
    return true;
}

// ==================== Thread d'echantillonnage ====================

static void boucleEchantillonnage()
{
    while (!d.echStop.loadAcquire()) {
        QThread::msleep(PERIODE_ECH_MS);

        qint64 w = d.chrono.nsecsElapsed();
        qint64 v = tempsVirtuel();

        Rafale r; qint64 egoWall = -1, egoVirt = -1; bool ferme = false;

        d.mutex.lock();
        if (v >= 0) {
            d.echs.append({w, v});
            while (d.echs.size() > NB_ECH_MAX) d.echs.removeFirst();

            // --- periode ou le temps virtuel cesse d'avancer ---
            // (ignore les gaps d'echantillonnage enormes : l'echantillonneur
            // est alors affame, la lecture dv ne dit rien)
            if (d.echs.size() >= 2) {
                const Echantillon &p = d.echs[d.echs.size() - 2];
                const Echantillon &c = d.echs[d.echs.size() - 1];
                qint64 dw = c.w - p.w;
                qint64 dv = c.v - p.v;

                if (!d.pauseVM && dv < SEUIL_FIGE_NS && dw > 0 && dw <= 500 * NS_PAR_MS) {
                    if (!d.rupture.ouverte) {
                        d.rupture.w0 = p.w;
                        d.rupture.v0 = p.v;
                        d.rupture.ouverte = true;
                        if (d.p_timersActifs)
                            d.rupture.timersActifs = d.p_timersActifs(QEMU_CLOCK_VIRTUAL);
                        if (d.p_echeance)
                            d.rupture.echeanceNs =
                                d.p_echeance(QEMU_CLOCK_VIRTUAL, QEMU_TIMER_ATTR_ALL);
                        QString echTxt;
                        if (d.rupture.echeanceNs == LLONG_MIN) echTxt = "?";
                        else if (d.rupture.echeanceNs < 0)     echTxt = "passee";
                        else echTxt = QString("%1 ms").arg(msec(d.rupture.echeanceNs), 0, 'f', 1);
                        trace(QString("temps virtuel fige : %1 (timers virtuels actifs : %2, prochaine echeance : %3)")
                                  .arg(relTemps(p.w))
                                  .arg(d.rupture.timersActifs ? "oui" : "non")
                                  .arg(echTxt));
                    }
                    d.rupture.w1 = c.w;
                    d.rupture.v1 = c.v;
                } else if (d.rupture.ouverte) {
                    d.rupture.ouverte = false;
                    qint64 duree = d.rupture.w1 - d.rupture.w0;
                    if (duree >= SEUIL_RUPT_NS) {
                        trace(QString("fin du temps virtuel fige : duree %1 ms (avance virtuelle %2 ms)")
                                  .arg(msec(duree), 0, 'f', 0)
                                  .arg(msec(d.rupture.v1 - d.rupture.v0), 0, 'f', 0));
                        d.ruptures.append(d.rupture);
                        while (d.ruptures.size() > NB_RUPT_MAX) d.ruptures.removeFirst();
                    }
                }
            }

            // --- VM lente : ratio virtuel/mur sur la fenetre recente ---
            if (!d.rupture.ouverte && d.echs.size() >= 25) {
                qint64 sw = 0, sv = 0;
                int debut = qMax(0, d.echs.size() - 25);
                for (int i = debut + 1; i < d.echs.size(); i++) {
                    sw += d.echs[i].w - d.echs[i - 1].w;
                    sv += d.echs[i].v - d.echs[i - 1].v;
                }
                if (sw > 0) {
                    double ratio = double(sv) / double(sw);
                    if (ratio < RATIO_LENT && !d.lentFlag) {
                        d.lentFlag = true;
                        trace(QString("VM lente : temps virtuel avance a x%1 du temps mur")
                                  .arg(ratio, 0, 'f', 2));
                    } else if (d.lentFlag && ratio > 0.9) {
                        d.lentFlag = false;
                        trace("fin de la lenteur de la VM : cadence virtuelle/mur normale");
                    }
                }
            }

            // --- rafale UART fermee si le silence suffise ---
            ferme = fermeRafaleSiSilencieuse(w, r, egoWall, egoVirt);
        }
        d.mutex.unlock();

        // hors mutex : le log ou le dump peut locker d'eux-memes
        if (ferme) ecartRafales(r, egoWall, egoVirt);
    }
}

} // namespace (anonyme)

// ==================== API publique ====================

void init(void *libHandle)
{
    d.lib = libHandle;
    d.chrono.start();
    d.wT0 = d.chrono.nsecsElapsed();
    d.echStop.storeRelease(0);

    const char *env = getenv("ESPSIMU_DIAG");
    d.actifMode = (env && env[0] == '1' && env[1] == '\0');

    if (!d.actifMode) {
        trace("inactif (definir ESPSIMU_DIAG=1 pour tracer le lag post-reset)");
        return;
    }

    if (libHandle) {
        d.p_tempsVirtuelNs = (t_qemu_clock_get_ns)          DIAG_SYM(libHandle, "qemu_clock_get_ns");
        d.p_timersActifs   = (t_qemu_clock_has_timers)      DIAG_SYM(libHandle, "qemu_clock_has_timers");
        d.p_echeance       = (t_qemu_clock_deadline_ns_all) DIAG_SYM(libHandle, "qemu_clock_deadline_ns_all");
    }
    trace(QString("ACTIF (temps virtuel : %1, etat timers : %2)")
              .arg(d.p_tempsVirtuelNs ? "disponible" : "NON RESOLU")
              .arg(d.p_timersActifs ? "dispo" : "absent"));
}

bool actif() { return d.actifMode; }

void mark(const char *label)
{
    if (!d.actifMode) return;
    qint64 v = tempsVirtuel();
    {
        QMutexLocker l(&d.mutex);
        qint64 w = d.chrono.nsecsElapsed();
        d.marques.append({w, v, label});
        while (d.marques.size() > NB_MARQ_MAX) d.marques.removeFirst();
        if (label && strncmp(label, "reset", 5) == 0) d.wDernierReset = w;
        if (label && strncmp(label, "pause", 5) == 0) d.pauseVM = true;
        if (label && strncmp(label, "resume", 6) == 0) d.pauseVM = false;
        trace(QString("marque %1 : %2, temps virtuel +%3s")
                  .arg(label).arg(relTemps(w))
                  .arg(v >= 0 ? QString::number(sec(v), 'f', 3) : "n/d"));
    }
}

// Thread QEMU : horodatage a la source de chaque octet UART emis.
void uartSource(int uartId, unsigned char octet)
{
    if (!d.actifMode) return;
    qint64 w = d.chrono.nsecsElapsed();
    qint64 v = tempsVirtuel();

    Rafale r; qint64 egoWall = -1, egoVirt = -1; bool ferme = false;

    d.mutex.lock();
    ferme = fermeRafaleSiSilencieuse(w, r, egoWall, egoVirt);

    if (d.rafCouranteId < 0) {
        d.rafCouranteId = ++d.compteurRafales;
        d.rafUart = uartId;
        d.rafWDeb = w;
        d.rafVDeb = v;
        d.rafN = 0;
        d.rafAperN = 0;
    }
    d.rafWFin = w;
    if (v >= 0) d.rafVFin = v;

    // apercu du contenu (caracteres affichables uniquement)
    if (octet >= 32 && octet < 127 && d.rafAperN < 24)
        d.rafApercu[d.rafAperN++] = (char)octet;
    d.rafN++;
    d.mutex.unlock();

    if (ferme) ecartRafales(r, egoWall, egoVirt);
}

// Thread appli : horodatage de la livraison UART cote Qt. Les rafales sont
// reconnues au vol (meme regle des 5 ms) et rapprochees de leurs emissions
// source par n° d'ordre ; l'ecart livraison-emission mesure la congestion
// de la file d'evenements Qt.
void uartLivraison(int uartId)
{
    if (!d.actifMode) return;
    qint64 w = d.chrono.nsecsElapsed();

    QMutexLocker l(&d.mutex);

    // cloture de la rafale Qt precedente si le silence a arrive
    if (d.qtCouranteId >= 0 && w - d.qtWFin >= GAP_RAF_NS) {
        int idTerminee = d.qtCouranteId;
        qint64 wDebQ = d.qtWDeb, wFinQ = d.qtWFin;
        d.qtCouranteId = -1;
        bool trouve = false;
        for (Rafale &cand : d.rafales) {
            if (cand.id != idTerminee) continue;
            cand.wDebQt = wDebQ;
            cand.wFinQt = wFinQ;
            trouve = true;
            break;
        }
        if (trouve && wFinQ - d.qtWFinPrec >= 0) {
            // retard de livraison appreciable ?
            for (const Rafale &cand : d.rafales) {
                if (cand.id != idTerminee) continue;
                if (cand.wFinS > 0 && wFinQ - cand.wFinS > SEUIL_LIVRAISON)
                    trace(QString("livraison Qt en retard : rafale n%1 arrivee %2 ms apres la fin d'emission")
                              .arg(idTerminee).arg(msec(wFinQ - cand.wFinS), 0, 'f', 0));
                break;
            }
        }
        d.qtWFinPrec = wFinQ;
    }

    if (d.qtCouranteId < 0) {
        // meme numerotation que les rafales source : id courant si ouvert,
        // sinon le prochain id attendu
        d.qtCouranteId = (d.rafCouranteId >= 0) ? d.rafCouranteId
                                                : d.compteurRafales + 1;
        d.qtWDeb = w;
    }
    d.qtWFin = w;
    Q_UNUSED(uartId);
}

void demarrerEchantillonneur()
{
    if (!d.actifMode || d.echThread) return;
    d.echStop.storeRelease(0);
    d.echThread = QThread::create([]() { boucleEchantillonnage(); });
    d.echThread->setObjectName("EspDiag-echantillonnage");
    d.echThread->start();
    trace("echantillonnage du temps virtuel demarre (20 ms)");
}

void arreterEchantillonneur()
{
    if (!d.echThread) return;
    d.echStop.storeRelease(1);
    d.echThread->wait(2000);
    delete d.echThread;
    d.echThread = nullptr;
}

void dump(const QString &raison)
{
    if (!d.actifMode) return;
    qint64 vRel = tempsVirtuel();
    QMutexLocker l(&d.mutex);
    qint64 w = d.chrono.nsecsElapsed();

    trace(QString("---- dump : %1 (%2, temps virtuel +%3s), %4 ech., %5 rafales ----")
              .arg(raison).arg(relTemps(w))
              .arg(vRel >= 0 ? QString::number(sec(vRel), 'f', 3) : "n/d")
              .arg(d.echs.size()).arg(d.rafales.size()));

    // derniers marqueurs
    int debutMarq = qMax(0, d.marques.size() - 8);
    for (int i = debutMarq; i < d.marques.size(); i++) {
        const Marque &m = d.marques[i];
        trace(QString("  marque %1 : %2, virtuel +%3s")
                  .arg(m.label).arg(relTemps(m.w))
                  .arg(m.v >= 0 ? QString::number(sec(m.v), 'f', 3) : "n/d"));
    }

    // periodes figees
    if (d.rupture.ouverte)
        trace(QString("rupture figee EN COURS depuis %1").arg(relTemps(d.rupture.w0)));
    for (int i = 0; i < d.ruptures.size(); i++) {
        const Rupture &r = d.ruptures[i];
        trace(QString("  fige %1 : %2 duree %3 ms (avance virtuelle %4 ms)")
                  .arg(i + 1)
                  .arg(relTemps(r.w0))
                  .arg(msec(r.w1 - r.w0), 0, 'f', 0)
                  .arg(msec(r.v1 - r.v0), 0, 'f', 0));
    }
    trace(QString("(total periodes figees loguees : %1)").arg(d.ruptures.size()));

    // dernieres rafales UART
    int debutRaf = qMax(0, d.rafales.size() - 40);
    for (int i = debutRaf; i < d.rafales.size(); i++) {
        const Rafale &r = d.rafales[i];
        const Rafale *prec = (i > 0) ? &d.rafales[i - 1] : nullptr;
        QString em;
        if (prec) {
            double mw = msec(r.wDebS - prec->wFinS) / 1000.0;
            double mv = msec(r.vDebS - prec->vFinS) / 1000.0;
            double mq = (r.wDebQt >= 0 && prec->wFinQt >= 0)
                            ? msec(r.wDebQt - prec->wFinQt) / 1000.0 : -1;
            em = QString("| mur %1 s | virt %2 s | Qt %3 s")
                     .arg(mw, 0, 'f', 2).arg(mv, 0, 'f', 2)
                     .arg(mq >= 0 ? QString::number(mq, 'f', 2) : "?");
        }
        trace(QString("  rafale n%1 uart%2, %3 oct., fin %4 %5 {%6}")
                  .arg(r.id).arg(r.uartId).arg(r.n)
                  .arg(relTemps(r.wFinS)).arg(em)
                  .arg(QString::fromLatin1(r.apercu)));
    }
    trace("---- fin dump ----");
}

} // namespace EspDiag
