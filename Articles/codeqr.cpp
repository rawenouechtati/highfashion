#include "codeqr.h"
#include <QPainter>
#include <algorithm>

namespace Art {

// ---- tables de la norme : octets de correction par bloc et nombre de blocs (index = version 1..20) ----
static const int ECC_PAR_BLOC[2][21] = {
    /* L */ {-1, 7, 10, 15, 20, 26, 18, 20, 24, 30, 18, 20, 24, 26, 30, 22, 24, 28, 30, 28, 28},
    /* M */ {-1, 10, 16, 26, 18, 24, 16, 18, 22, 22, 26, 30, 22, 22, 24, 24, 28, 28, 26, 26, 26}};
static const int NB_BLOCS[2][21] = {
    /* L */ {-1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 4, 4, 6, 6, 6, 6, 7, 8},
    /* M */ {-1, 1, 1, 1, 2, 2, 4, 4, 4, 5, 5, 5, 8, 9, 9, 10, 10, 11, 13, 14, 16}};
static const int BITS_FORMAT_NIVEAU[2] = {1, 0};   // L = 01, M = 00

// nombre de modules disponibles pour les données + correction d'erreurs (hors motifs fixes)
static int modulesDeDonnees(int ver)
{
    int res = (16 * ver + 128) * ver + 64;
    if (ver >= 2) {
        const int nbAlign = ver / 7 + 2;
        res -= (25 * nbAlign - 10) * nbAlign - 55;
        if (ver >= 7) res -= 36;
    }
    return res;
}

static int octetsDeDonnees(int ver, int niveau)
{
    return modulesDeDonnees(ver) / 8 - ECC_PAR_BLOC[niveau][ver] * NB_BLOCS[niveau][ver];
}

// ---- Reed-Solomon sur GF(256) (polynôme 0x11D) ----
static quint8 multiplier(quint8 x, quint8 y)
{
    int z = 0;
    for (int i = 7; i >= 0; --i) {
        z = (z << 1) ^ ((z >> 7) * 0x11D);
        z ^= ((y >> i) & 1) * x;
    }
    return static_cast<quint8>(z);
}

static QVector<quint8> diviseurRS(int degre)
{
    QVector<quint8> res(degre, 0);
    res[degre - 1] = 1;
    quint8 racine = 1;
    for (int i = 0; i < degre; ++i) {
        for (int j = 0; j < degre; ++j) {
            res[j] = multiplier(res[j], racine);
            if (j + 1 < degre) res[j] ^= res[j + 1];
        }
        racine = multiplier(racine, 0x02);
    }
    return res;
}

static QVector<quint8> resteRS(const QVector<quint8> &donnees, const QVector<quint8> &diviseur)
{
    QVector<quint8> res(diviseur.size(), 0);
    for (quint8 b : donnees) {
        const quint8 facteur = b ^ res.first();
        res.removeFirst();
        res.append(0);
        for (int i = 0; i < res.size(); ++i) res[i] ^= multiplier(diviseur[i], facteur);
    }
    return res;
}

static QVector<int> positionsAlignement(int ver, int taille)
{
    QVector<int> res;
    if (ver == 1) return res;
    const int nb = ver / 7 + 2;
    const int pas = (ver == 32) ? 26 : (ver * 4 + nb * 2 + 1) / (nb * 2 - 2) * 2;
    for (int i = 0, pos = taille - 7; i < nb - 1; ++i, pos -= pas) res.prepend(pos);
    res.prepend(6);
    return res;
}

bool CodeQr::module(int x, int y) const
{
    return x >= 0 && y >= 0 && x < taille_ && y < taille_ && modules_[y * taille_ + x];
}

// ---------------------------------------------------------------------------------------------
bool CodeQr::encoder(const QByteArray &octets, CodeQr &q, Correction niveau)
{
    const int niv = static_cast<int>(niveau);
    int ver = 0;
    for (int v = 1; v <= 20; ++v) {
        const int bitsCompteur = v < 10 ? 8 : 16;
        const int bitsNecessaires = 4 + bitsCompteur + 8 * octets.size();
        if (bitsNecessaires <= octetsDeDonnees(v, niv) * 8) { ver = v; break; }
    }
    if (ver == 0) return false;

    // 1) flux de bits : indicateur de mode « octets » (0100) + compteur + données + terminateur + remplissage
    QVector<bool> bits;
    auto ajouter = [&bits](quint32 val, int nb) { for (int i = nb - 1; i >= 0; --i) bits.append((val >> i) & 1); };
    ajouter(0x4, 4);
    ajouter(octets.size(), ver < 10 ? 8 : 16);
    for (char c : octets) ajouter(static_cast<quint8>(c), 8);
    const int capaciteBits = octetsDeDonnees(ver, niv) * 8;
    ajouter(0, std::min<int>(4, capaciteBits - static_cast<int>(bits.size())));
    ajouter(0, static_cast<int>((8 - bits.size() % 8) % 8));
    for (quint8 pad = 0xEC; static_cast<int>(bits.size()) < capaciteBits; pad ^= 0xEC ^ 0x11) ajouter(pad, 8);
    QVector<quint8> mots(bits.size() / 8, 0);
    for (int i = 0; i < bits.size(); ++i) mots[i >> 3] |= bits[i] << (7 - (i & 7));

    // 2) découpe en blocs, calcul de la correction d'erreurs, entrelacement
    const int nbBlocs = NB_BLOCS[niv][ver], eccBloc = ECC_PAR_BLOC[niv][ver];
    const int brut = modulesDeDonnees(ver) / 8;
    const int nbCourts = nbBlocs - brut % nbBlocs, longCourt = brut / nbBlocs;
    const QVector<quint8> diviseur = diviseurRS(eccBloc);
    QVector<QVector<quint8>> blocs;
    for (int i = 0, k = 0; i < nbBlocs; ++i) {
        const int lenDonnees = longCourt - eccBloc + (i < nbCourts ? 0 : 1);
        QVector<quint8> d = mots.mid(k, lenDonnees);
        k += lenDonnees;
        const QVector<quint8> ecc = resteRS(d, diviseur);
        if (i < nbCourts) d.append(0);   // case vide pour que tous les blocs aient la même longueur
        d += ecc;
        blocs.append(d);
    }
    QVector<quint8> final;
    for (int i = 0; i < blocs[0].size(); ++i)
        for (int j = 0; j < nbBlocs; ++j)
            if (i != longCourt - eccBloc || j >= nbCourts) final.append(blocs[j][i]);

    // 3) matrice : motifs fixes, données, meilleur masque
    q.version_ = ver;
    q.taille_ = ver * 4 + 17;
    q.modules_.fill(0, q.taille_ * q.taille_);
    q.fonction_.fill(0, q.taille_ * q.taille_);
    q.tracerMotifs(BITS_FORMAT_NIVEAU[niv]);
    q.placerDonnees(final);
    int meilleur = 0;
    long mini = -1;
    for (int m = 0; m < 8; ++m) {
        q.appliquerMasque(m);
        q.tracerFormat(BITS_FORMAT_NIVEAU[niv], m);
        const long p = q.penalite();
        if (mini < 0 || p < mini) { mini = p; meilleur = m; }
        q.appliquerMasque(m);   // un 2e XOR annule le masque
    }
    q.appliquerMasque(meilleur);
    q.tracerFormat(BITS_FORMAT_NIVEAU[niv], meilleur);
    return true;
}

void CodeQr::tracerMotifs(int niveauFormat)
{
    for (int i = 0; i < taille_; ++i) {      // lignes de synchronisation
        fixer(6, i, i % 2 == 0);
        fixer(i, 6, i % 2 == 0);
    }
    auto viseur = [this](int cx, int cy) {   // motif de repérage 7x7 + séparateur blanc
        for (int dy = -4; dy <= 4; ++dy)
            for (int dx = -4; dx <= 4; ++dx) {
                const int dist = std::max(std::abs(dx), std::abs(dy));
                const int x = cx + dx, y = cy + dy;
                if (x >= 0 && x < taille_ && y >= 0 && y < taille_) fixer(x, y, dist != 2 && dist != 4);
            }
    };
    viseur(3, 3);
    viseur(taille_ - 4, 3);
    viseur(3, taille_ - 4);
    const QVector<int> pos = positionsAlignement(version_, taille_);
    const int n = pos.size();
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
            if ((i == 0 && j == 0) || (i == 0 && j == n - 1) || (i == n - 1 && j == 0)) continue;
            for (int dy = -2; dy <= 2; ++dy)
                for (int dx = -2; dx <= 2; ++dx)
                    fixer(pos[i] + dx, pos[j] + dy, std::max(std::abs(dx), std::abs(dy)) != 1);
        }
    tracerFormat(niveauFormat, 0);           // réserve les cases du format (réécrites avec le vrai masque)
    if (version_ >= 7) {                     // informations de version (BCH 18,6)
        int reste = version_;
        for (int i = 0; i < 12; ++i) reste = (reste << 1) ^ ((reste >> 11) * 0x1F25);
        const long bitsVersion = static_cast<long>(version_) << 12 | reste;
        for (int i = 0; i < 18; ++i) {
            const bool noir = (bitsVersion >> i) & 1;
            const int a = taille_ - 11 + i % 3, b = i / 3;
            fixer(a, b, noir);
            fixer(b, a, noir);
        }
    }
}

void CodeQr::tracerFormat(int niveauFormat, int masque)
{
    const int donnees = niveauFormat << 3 | masque;
    int reste = donnees;
    for (int i = 0; i < 10; ++i) reste = (reste << 1) ^ ((reste >> 9) * 0x537);
    const int bitsFormat = (donnees << 10 | reste) ^ 0x5412;
    auto bit = [bitsFormat](int i) { return ((bitsFormat >> i) & 1) != 0; };
    for (int i = 0; i <= 5; ++i) fixer(8, i, bit(i));
    fixer(8, 7, bit(6));
    fixer(8, 8, bit(7));
    fixer(7, 8, bit(8));
    for (int i = 9; i < 15; ++i) fixer(14 - i, 8, bit(i));
    for (int i = 0; i < 8; ++i) fixer(taille_ - 1 - i, 8, bit(i));
    for (int i = 8; i < 15; ++i) fixer(8, taille_ - 15 + i, bit(i));
    fixer(8, taille_ - 8, true);             // module noir fixe
}

void CodeQr::placerDonnees(const QVector<quint8> &mots)
{
    int i = 0;
    const int total = mots.size() * 8;
    for (int droite = taille_ - 1; droite >= 1; droite -= 2) {
        if (droite == 6) droite = 5;
        for (int vert = 0; vert < taille_; ++vert)
            for (int j = 0; j < 2; ++j) {
                const int x = droite - j;
                const bool montee = ((droite + 1) & 2) == 0;
                const int y = montee ? taille_ - 1 - vert : vert;
                if (!fonction_[y * taille_ + x] && i < total) {
                    modules_[y * taille_ + x] = (mots[i >> 3] >> (7 - (i & 7))) & 1;
                    ++i;
                }
            }
    }
}

void CodeQr::appliquerMasque(int masque)
{
    for (int y = 0; y < taille_; ++y)
        for (int x = 0; x < taille_; ++x) {
            bool inv = false;
            switch (masque) {
            case 0: inv = (x + y) % 2 == 0; break;
            case 1: inv = y % 2 == 0; break;
            case 2: inv = x % 3 == 0; break;
            case 3: inv = (x + y) % 3 == 0; break;
            case 4: inv = (x / 3 + y / 2) % 2 == 0; break;
            case 5: inv = x * y % 2 + x * y % 3 == 0; break;
            case 6: inv = (x * y % 2 + x * y % 3) % 2 == 0; break;
            default: inv = ((x + y) % 2 + x * y % 3) % 2 == 0; break;
            }
            if (inv && !fonction_[y * taille_ + x]) modules_[y * taille_ + x] ^= 1;
        }
}

// Pénalités de la norme (suites de modules identiques, blocs 2x2, motifs de repérage, équilibre noir/blanc)
long CodeQr::penalite() const
{
    long res = 0;
    auto m = [this](int x, int y) { return modules_[y * taille_ + x] != 0; };
    for (int sens = 0; sens < 2; ++sens)
        for (int a = 0; a < taille_; ++a) {
            int serie = 1;
            QVector<bool> ligne;
            for (int b = 0; b < taille_; ++b) {
                const bool cur = sens == 0 ? m(b, a) : m(a, b);
                ligne.append(cur);
                if (b > 0) {
                    const bool prec = sens == 0 ? m(b - 1, a) : m(a, b - 1);
                    if (cur == prec) { ++serie; if (serie == 5) res += 3; else if (serie > 5) ++res; }
                    else serie = 1;
                }
            }
            static const bool P1[11] = {1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0}, P2[11] = {0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1};
            for (int b = 0; b + 11 <= taille_; ++b) {
                bool ok1 = true, ok2 = true;
                for (int k = 0; k < 11; ++k) { ok1 &= ligne[b + k] == P1[k]; ok2 &= ligne[b + k] == P2[k]; }
                if (ok1) res += 40;
                if (ok2) res += 40;
            }
        }
    for (int y = 0; y + 1 < taille_; ++y)
        for (int x = 0; x + 1 < taille_; ++x) {
            const bool c = m(x, y);
            if (c == m(x + 1, y) && c == m(x, y + 1) && c == m(x + 1, y + 1)) res += 3;
        }
    long noirs = 0;
    for (quint8 v : modules_) noirs += v;
    const long total = static_cast<long>(taille_) * taille_;
    const long k = (std::labs(noirs * 20 - total * 10) + total - 1) / total - 1;
    res += std::max(0L, k) * 10;
    return res;
}

QImage CodeQr::image(int px, int bordure) const
{
    const int cote = (taille_ + 2 * bordure) * px;
    QImage img(cote, cote, QImage::Format_RGB32);
    img.fill(Qt::white);
    QPainter p(&img);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    for (int y = 0; y < taille_; ++y)
        for (int x = 0; x < taille_; ++x)
            if (module(x, y)) p.drawRect((x + bordure) * px, (y + bordure) * px, px, px);
    return img;
}

}  // namespace Art
