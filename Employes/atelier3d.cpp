#include "atelier3d.h"
#include "pictogrammes.h"
#include "posteTravail.h"

#include <QFontMetricsF>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRadialGradient>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace {

const char *kMarine = "#1F3A4D";
const char *kBordeaux = "#8B1E2F";
const char *kVert = "#4F7857";
const char *kGris = "#8A8375";

const qreal kPi = 3.14159265358979;
const int kHauteurBandeau = 58;
const qreal kYawDefaut = 18;
const qreal kPitchDefaut = 36;
const float kEpaisseurPad = 0.08f;
const float kLargeurPad = 4.9f;
const float kProfondeurPad = 4.3f;
const float kPasX = 5.5f;
const float kPasZ = 5.0f;

QVector3D V(float x, float y, float z) { return QVector3D(x, y, z); }

// position d'un evenement souris / molette dans le widget (Qt 5 et Qt 6)
QPointF positionDe(const QMouseEvent *e)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return e->position();
#else
    return e->localPos();
#endif
}

QPointF positionDe(const QWheelEvent *e)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return e->position();
#else
    return e->posF();
#endif
}

// couleur multipliee par un facteur d'eclairage (1 = inchangee)
QColor nuance(const QColor &c, qreal k)
{
    return QColor(qBound(0, int(c.red() * k), 255), qBound(0, int(c.green() * k), 255),
                  qBound(0, int(c.blue() * k), 255), c.alpha());
}

// sens de la lumiere : en haut a gauche, un peu de face
const QVector3D kLumiere = QVector3D(-0.406f, 0.767f, 0.496f);

Atelier3D::Face creerFace(const QVector<QVector3D> &pts, const QVector3D &centreSolide)
{
    Atelier3D::Face f;
    f.points = pts;
    QVector3D c(0, 0, 0);
    for (int i = 0; i < pts.size(); ++i)
        c += pts[i];
    c /= float(pts.size());
    f.centre = c;
    QVector3D n = QVector3D::crossProduct(pts[1] - pts[0], pts[pts.size() - 1] - pts[0]);
    n.normalize();
    if (QVector3D::dotProduct(n, c - centreSolide) < 0)
        n = -n;                    // la normale regarde toujours vers l'exterieur du solide
    f.normale = n;
    return f;
}

// Prisme convexe : le polygone 'base' (plan XZ, relatif a (cx, cz)) est extrude de y0 a y1 ; le
// dessus est reduit/agrandi de 'echelleHaut' (1 = prisme droit). Le dessous n'est jamais visible.
Atelier3D::Solide prisme(const QVector<QPointF> &base, float cx, float cz, float y0, float y1,
                         float echelleHaut, const QColor &couleur)
{
    Atelier3D::Solide s;
    s.couleur = couleur;
    s.centre = V(cx, (y0 + y1) / 2, cz);
    const int n = base.size();
    QVector<QVector3D> bas, haut;
    for (int i = 0; i < n; ++i) {
        bas.append(V(cx + float(base[i].x()), y0, cz + float(base[i].y())));
        haut.append(V(cx + float(base[i].x()) * echelleHaut, y1, cz + float(base[i].y()) * echelleHaut));
    }
    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        QVector<QVector3D> quad;
        quad << bas[i] << bas[j] << haut[j] << haut[i];
        s.faces.append(creerFace(quad, s.centre));
    }
    s.faces.append(creerFace(haut, s.centre));
    return s;
}

Atelier3D::Solide boite(float cx, float cz, float l, float p, float y0, float y1, const QColor &c, float rot = 0)
{
    QVector<QPointF> base;
    const float xs[4] = {-1, 1, 1, -1};
    const float zs[4] = {-1, -1, 1, 1};
    const float cs = std::cos(rot), sn = std::sin(rot);
    for (int i = 0; i < 4; ++i) {
        const float x = xs[i] * l / 2, z = zs[i] * p / 2;
        base.append(QPointF(x * cs - z * sn, x * sn + z * cs));
    }
    return prisme(base, cx, cz, y0, y1, 1.0f, c);
}

Atelier3D::Solide cylindre(float cx, float cz, float r, float y0, float y1, const QColor &c,
                           float echelleHaut = 1.0f, int segments = 16)
{
    QVector<QPointF> base;
    for (int i = 0; i < segments; ++i) {
        const double a = 2 * kPi * i / segments;
        base.append(QPointF(r * std::cos(a), r * std::sin(a)));
    }
    return prisme(base, cx, cz, y0, y1, echelleHaut, c);
}

// plateau + 4 pieds : table de travail
void ajouterTable(QVector<Atelier3D::Solide> &d, float cx, float cz, float l, float p, float h, const QColor &bois)
{
    const QColor pied = nuance(bois, 0.78);
    const float dx = l / 2 - 0.16f, dz = p / 2 - 0.16f;
    d << boite(cx - dx, cz - dz, 0.18f, 0.18f, 0, h - 0.14f, pied);
    d << boite(cx + dx, cz - dz, 0.18f, 0.18f, 0, h - 0.14f, pied);
    d << boite(cx - dx, cz + dz, 0.18f, 0.18f, 0, h - 0.14f, pied);
    d << boite(cx + dx, cz + dz, 0.18f, 0.18f, 0, h - 0.14f, pied);
    d << boite(cx, cz, l, p, h - 0.14f, h, bois);
}

QString initialesCourtes(const Employe &e)
{
    QString nom = e.nom().trimmed();
    return e.prenom().trimmed() + (nom.isEmpty() ? QString() : " " + nom.left(1).toUpper() + ".");
}

} // namespace

struct Atelier3D::Element {
    float profondeur;
    const Solide *solide;      // nullptr = tete d'un personnage
    int poste;
    QVector3D tete;
    float alpha;
    QColor cheveux;
};

// ------------------------------------------------------------------------------------
// Construction
// ------------------------------------------------------------------------------------
Atelier3D::Atelier3D(bool actions, QWidget *parent)
    : QWidget(parent), m_actions(actions)
{
    setObjectName("atelier3d");
    setMouseTracking(true);
    setCursor(Qt::OpenHandCursor);
    m_yaw = m_yawCible = kYawDefaut;
    m_pitch = m_pitchCible = kPitchDefaut;
    m_depuisAction.start();
    m_horloge.start();

    // --- bandeau du bas : fiche du poste survole / choisi ---
    m_bandeau = new QFrame(this);
    m_bandeau->setObjectName("bandeau3d");
    m_bandeau->setFixedHeight(kHauteurBandeau);
    m_bandeau->setStyleSheet("QFrame#bandeau3d { background-color: #FFFDF9; border: 1px solid #EADFD0; border-radius: 10px; }");
    auto *ligne = new QHBoxLayout(m_bandeau);
    ligne->setContentsMargins(12, 6, 12, 6);
    ligne->setSpacing(12);
    m_pastille = new QLabel();
    m_pastille->setFixedSize(42, 42);
    m_pastille->setAlignment(Qt::AlignCenter);
    ligne->addWidget(m_pastille);
    auto *textes = new QVBoxLayout();
    textes->setSpacing(0);
    m_titre = new QLabel();
    m_titre->setStyleSheet(QString("font-family: 'Playfair Display'; font-weight: bold; font-size: 11pt; color: %1; background: transparent;").arg(kMarine));
    m_detail = new QLabel();
    m_detail->setStyleSheet(QString("font-size: 9pt; color: %1; background: transparent;").arg(kGris));
    textes->addWidget(m_titre);
    textes->addWidget(m_detail);
    ligne->addLayout(textes, 1);
    m_badge = new QLabel();
    m_badge->setAlignment(Qt::AlignCenter);
    m_badge->setFixedHeight(22);
    ligne->addWidget(m_badge);
    m_action = new QPushButton();
    m_action->setCursor(Qt::PointingHandCursor);
    m_action->setMinimumHeight(32);
    m_action->hide();
    ligne->addWidget(m_action);

    auto *racine = new QVBoxLayout(this);
    racine->setContentsMargins(0, 0, 0, 0);
    racine->setSpacing(0);
    racine->addStretch(1);
    racine->addWidget(m_bandeau);

    m_boutonVue = new QToolButton(this);
    m_boutonVue->setText("Vue d'ensemble");
    m_boutonVue->setCursor(Qt::PointingHandCursor);
    m_boutonVue->setStyleSheet(QString(
        "QToolButton { background-color: #F3ECE2; color: %1; border: none; border-radius: 11px; padding: 3px 12px; font-size: 8pt; }"
        "QToolButton:hover { background-color: #E7DAC8; }").arg(kMarine));
    m_boutonVue->adjustSize();
    connect(m_boutonVue, &QToolButton::clicked, this, [this]() { reinitialiserVue(); });

    connect(m_action, &QPushButton::clicked, this, [this]() {
        if (m_selection < 0 || m_selection >= m_postes.size())
            return;
        const QString poste = m_postes[m_selection];
        emit changerPosteDemande(poste == m_monPoste ? QString() : poste);
    });

    m_minuteur = new QTimer(this);
    m_minuteur->setInterval(33);
    connect(m_minuteur, &QTimer::timeout, this, [this]() { surTic(); });

    majBandeau();
}

QVector3D Atelier3D::positionPoste(int index) const
{
    return m_centres.value(index);
}

void Atelier3D::setServices(const QMap<int, QString> &services)
{
    m_services = services;
    majBandeau();
}

void Atelier3D::setFiltre(int filtre)
{
    m_filtre = filtre;
}

QString Atelier3D::posteSelectionne() const
{
    return (m_selection >= 0 && m_selection < m_postes.size()) ? m_postes[m_selection] : QString();
}

void Atelier3D::selectionnerPoste(const QString &poste)
{
    const int i = m_postes.indexOf(poste);
    m_selection = i;
    majBandeau();
    emit posteChoisi(i >= 0 ? poste : QString());
    update();
}

void Atelier3D::reinitialiserVue()
{
    m_yawCible = kYawDefaut;
    m_pitchCible = kPitchDefaut;
    m_zoomCible = 1.0;
    m_depuisAction.restart();
}

void Atelier3D::setDonnees(const QStringList &postes, const QMap<QString, Employe> &occupants,
                           const QString &monPoste)
{
    QString signature = "moi:" + monPoste;
    for (int i = 0; i < postes.size(); ++i) {
        const Employe e = occupants.value(postes[i]);
        signature += "|" + postes[i] + ";" + QString::number(e.id()) + ";" + e.prenom() + ";" + e.nom()
                     + ";" + e.poste() + ";" + QString::number(e.idService());
    }
    if (signature == m_signature)
        return;     // rien n'a change : on ne perturbe ni l'animation ni la selection
    m_signature = signature;

    const QString selectionAvant = posteSelectionne();
    const bool premiere = m_postes.isEmpty();
    const bool memePostes = (postes == m_postes);
    m_occupants = occupants;
    m_monPoste = monPoste;
    if (!memePostes) {
        m_postes = postes;
        construireScene();
    }
    m_selection = m_postes.indexOf(selectionAvant);
    if (premiere) {         // premier affichage : les employes sont deja la, pas de fondu
        for (int i = 0; i < m_postes.size(); ++i)
            m_etats[i].presence = m_occupants.contains(m_postes[i]) ? 1.0 : 0.0;
        m_distance = distanceAjustee(m_yaw, m_pitch, zoneScene()) / m_zoom;
    }
    majBandeau();
    update();
}

// ------------------------------------------------------------------------------------
// Scene : implantation des postes et objets fixes
// ------------------------------------------------------------------------------------
void Atelier3D::construireScene()
{
    const int n = m_postes.size();
    const int colonnes = qMax(1, qMin(3, n));
    const int lignes = qMax(1, (n + 2) / 3);
    m_demiLargeur = colonnes * kPasX / 2.0 + 0.8;
    m_demiProfondeur = lignes * kPasZ / 2.0 + 0.7;

    m_decor = QVector<QVector<Solide> >(n);
    m_etats = QVector<Etat>(n);
    m_centres = QVector<QVector3D>(n);
    for (int i = 0; i < n; ++i) {
        const int col = i % 3, lig = i / 3;
        const float x = (col - (qMin(3, n) - 1) / 2.0f) * kPasX;
        const float z = (lig - (lignes - 1) / 2.0f) * kPasZ;
        m_centres[i] = V(x, 0, z);
        construireDecorPoste(i, PosteTravail::type(m_postes[i]), m_centres[i]);
    }
}

void Atelier3D::construireDecorPoste(int index, const QString &type, const QVector3D &c)
{
    QVector<Solide> &d = m_decor[index];
    const float x = c.x(), z = c.z();
    const QColor bois("#B98F62"), metal("#3C3F4A"), acier("#D5DADF"), creme("#F3EBDC");
    const QColor rouge("#A6303F"), bleu("#3E6C8F"), jaune("#E0B64A");

    if (type == "Couture") {
        ajouterTable(d, x, z + 0.3f, 3.3f, 1.7f, 1.5f, bois);
        d << boite(x - 0.15f, z + 0.3f, 1.35f, 0.75f, 1.5f, 1.68f, metal);            // socle de la machine
        d << boite(x + 0.38f, z + 0.3f, 0.34f, 0.42f, 1.68f, 2.6f, acier);            // colonne
        d << boite(x - 0.1f, z + 0.3f, 1.2f, 0.42f, 2.3f, 2.62f, acier);              // bras
        d << boite(x - 0.62f, z + 0.3f, 0.32f, 0.4f, 1.68f, 2.62f, nuance(acier, 0.82)); // tete
        d << cylindre(x + 0.1f, z + 0.3f, 0.15f, 2.62f, 2.95f, rouge, 1.0f, 10);      // bobine
        d << boite(x - 1.2f, z + 0.5f, 0.95f, 1.0f, 1.5f, 1.56f, rouge);              // tissu en cours
        d << boite(x + 1.15f, z + 0.55f, 0.5f, 0.5f, 1.5f, 1.72f, creme);             // pelote de fil
    } else if (type == "Nettoyage") {
        d << boite(x, z + 0.2f, 2.4f, 1.8f, 0, 2.4f, QColor("#DDE3E8"));              // lave-linge industriel
        d << boite(x, z - 0.55f, 2.4f, 0.5f, 2.4f, 2.65f, QColor("#B9C2CA"));         // tableau de commande
        d << boite(x, z + 1.12f, 1.1f, 0.1f, 0.85f, 1.95f, QColor("#35566B"));        // hublot
        d << boite(x - 0.7f, z + 1.12f, 0.35f, 0.12f, 2.05f, 2.25f, QColor("#C0443E")); // voyant
        d << boite(x + 1.85f, z + 0.5f, 0.95f, 0.85f, 0, 0.7f, QColor("#D98C3F"));    // panier a linge
        d << boite(x + 1.85f, z + 0.5f, 0.78f, 0.7f, 0.7f, 0.85f, QColor("#F3EBDC"));
        d << boite(x - 1.9f, z + 0.9f, 0.8f, 0.7f, 0, 0.45f, QColor("#8FB4C9"));      // bac
    } else if (type == "Authentification") {
        ajouterTable(d, x, z + 0.3f, 3.3f, 1.7f, 1.5f, bois);
        d << boite(x - 0.95f, z + 0.45f, 1.1f, 1.45f, 1.5f, 1.54f, QColor("#FBF8F2")); // certificat
        d << cylindre(x - 0.7f, z + 0.85f, 0.2f, 1.54f, 1.6f, rouge, 1.0f, 12);        // sceau
        d << cylindre(x + 0.1f, z + 0.45f, 0.5f, 1.5f, 1.58f, QColor("#BFE0F0"), 1.0f, 18); // loupe
        d << boite(x + 0.75f, z + 0.9f, 0.95f, 0.14f, 1.5f, 1.62f, metal, 0.7f);       // manche
        d << cylindre(x + 1.25f, z - 0.2f, 0.28f, 1.5f, 1.62f, metal, 1.0f, 12);       // lampe : pied
        d << cylindre(x + 1.25f, z - 0.2f, 0.06f, 1.62f, 3.0f, metal, 1.0f, 8);        // bras
        d << cylindre(x + 1.25f, z - 0.2f, 0.5f, 2.95f, 3.4f, jaune, 0.45f, 14);       // abat-jour
        d << boite(x - 0.2f, z + 0.15f, 0.9f, 0.6f, 1.5f, 1.58f, QColor("#2B2F3A"));   // tablette
    } else if (type == "Pressing") {
        d << boite(x, z + 0.3f, 3.5f, 1.2f, 1.35f, 1.48f, QColor("#EDE7DA"));          // planche a repasser
        d << boite(x - 1.15f, z + 0.3f, 0.12f, 1.0f, 0, 1.35f, metal, 0.35f);          // pieds
        d << boite(x + 1.15f, z + 0.3f, 0.12f, 1.0f, 0, 1.35f, metal, -0.35f);
        d << boite(x - 0.4f, z + 0.3f, 1.9f, 0.95f, 1.48f, 1.54f, bleu);                // chemise
        d << boite(x - 1.35f, z + 0.3f, 0.7f, 0.4f, 1.48f, 1.56f, nuance(bleu, 0.9f));  // manche
        QVector<QPointF> fer;
        fer << QPointF(-0.5, -0.27) << QPointF(0.35, -0.27) << QPointF(0.62, 0) << QPointF(0.35, 0.27) << QPointF(-0.5, 0.27);
        d << prisme(fer, x + 1.05f, z + 0.3f, 1.48f, 1.82f, 0.82f, QColor("#C9CED6"));  // fer
        d << boite(x + 0.95f, z + 0.3f, 0.6f, 0.12f, 1.82f, 2.05f, metal);              // poignee
        d << boite(x + 1.3f, z - 1.0f, 0.5f, 0.5f, 0, 1.0f, QColor("#9FB8C8"));         // cuve de vapeur
    } else if (type == "Controle qualite") {
        d << cylindre(x - 1.2f, z + 0.3f, 0.55f, 0, 0.12f, metal, 1.0f, 16);            // mannequin : socle
        d << cylindre(x - 1.2f, z + 0.3f, 0.07f, 0.12f, 1.5f, metal, 1.0f, 8);          // mat
        d << cylindre(x - 1.2f, z + 0.3f, 0.95f, 0.9f, 2.0f, QColor("#9B2D3F"), 0.52f, 16); // jupe
        d << cylindre(x - 1.2f, z + 0.3f, 0.5f, 2.0f, 2.85f, QColor("#9B2D3F"), 1.25f, 14); // buste habille
        d << cylindre(x - 1.2f, z + 0.3f, 0.14f, 2.85f, 3.15f, QColor("#E7D6B8"), 1.0f, 8); // cou
        d << cylindre(x - 1.2f, z + 0.3f, 0.2f, 3.15f, 3.3f, QColor("#E7D6B8"), 0.4f, 8);
        ajouterTable(d, x + 1.15f, z + 0.35f, 1.9f, 1.3f, 1.5f, bois);
        d << boite(x + 1.15f, z + 0.3f, 0.75f, 1.0f, 1.5f, 1.55f, QColor("#FBF8F2"));    // fiche de controle
        d << boite(x + 1.15f, z + 0.1f, 0.3f, 0.12f, 1.55f, 1.6f, QColor(kVert));         // coche
    } else {
        ajouterTable(d, x, z + 0.3f, 3.0f, 1.6f, 1.5f, bois);
    }
}

// ------------------------------------------------------------------------------------
// Personnage : un employe qui occupe un poste (jambes, buste, bras ; la tete est dessinee a part)
// ------------------------------------------------------------------------------------
QVector<Atelier3D::Solide> Atelier3D::personnage(int, const QVector3D &c, qreal montee, qreal balancement,
                                                 const QColor &tenue) const
{
    QVector<Solide> s;
    const float x = c.x(), z = c.z() - 1.6f;
    const float h = float(montee + balancement);
    const QColor pantalon("#2E2F3A");
    s << boite(x - 0.26f, z, 0.36f, 0.36f, h, h + 1.45f, pantalon);
    s << boite(x + 0.26f, z, 0.36f, 0.36f, h, h + 1.45f, pantalon);
    QVector<QPointF> base;
    base << QPointF(-0.5, -0.28) << QPointF(0.5, -0.28) << QPointF(0.5, 0.28) << QPointF(-0.5, 0.28);
    s << prisme(base, x, z, h + 1.45f, h + 2.9f, 1.2f, tenue);
    const QColor manche = nuance(tenue, 0.88);
    s << boite(x - 0.74f, z + 0.05f, 0.26f, 0.28f, h + 1.7f, h + 2.8f, manche);
    s << boite(x + 0.74f, z + 0.05f, 0.26f, 0.28f, h + 1.7f, h + 2.8f, manche);
    return s;
}

QColor Atelier3D::couleurAccent(int poste) const
{
    if (poste < 0 || poste >= m_postes.size())
        return QColor(kGris);
    if (!m_monPoste.isEmpty() && m_postes[poste] == m_monPoste)
        return QColor(kMarine);
    return m_occupants.contains(m_postes[poste]) ? QColor(kBordeaux) : QColor(kVert);
}

// ------------------------------------------------------------------------------------
// Camera
// ------------------------------------------------------------------------------------
QRectF Atelier3D::zoneScene() const
{
    return QRectF(0, 0, width(), qMax(60, height() - kHauteurBandeau - 8));
}

void Atelier3D::preparerCamera(Camera &cam, qreal yaw, qreal pitch, qreal distance, const QRectF &zone,
                               bool avecDecalage) const
{
    const qreal a = yaw * kPi / 180.0, b = pitch * kPi / 180.0;
    const QVector3D cible(0, 0.9f, 0);
    cam.oeil = cible + QVector3D(float(distance * std::cos(b) * std::sin(a)), float(distance * std::sin(b)),
                                 float(distance * std::cos(b) * std::cos(a)));
    cam.vue.setToIdentity();
    cam.vue.lookAt(cam.oeil, cible, QVector3D(0, 1, 0));
    QMatrix4x4 proj;
    proj.perspective(30.0f, float(zone.width() / qMax<qreal>(1, zone.height())), 1.0f, 400.0f);
    cam.mvp = proj * cam.vue;
    cam.zone = zone;
    cam.decalage = QPointF(0, 0);
    cam.droite = QVector3D::crossProduct((cible - cam.oeil).normalized(), QVector3D(0, 1, 0)).normalized();
    cam.valide = true;
    if (avecDecalage) {
        const QRectF e = encombrement(cam);
        if (e.isValid())
            cam.decalage = QPointF(zone.center().x() - e.center().x(),
                                   (zone.top() + 40 + zone.bottom() - 30) / 2.0 - e.center().y());
    }
}

bool Atelier3D::projeter(const Camera &cam, const QVector3D &point, QPointF *ecran) const
{
    const QVector4D c = cam.mvp * QVector4D(point, 1.0f);
    if (c.w() <= 0.05f)
        return false;
    const qreal nx = c.x() / c.w(), ny = c.y() / c.w();
    ecran->setX(cam.zone.left() + (nx * 0.5 + 0.5) * cam.zone.width() + cam.decalage.x());
    ecran->setY(cam.zone.top() + (1.0 - (ny * 0.5 + 0.5)) * cam.zone.height() + cam.decalage.y());
    return true;
}

// rectangle (ecran) qui englobe le sol, les personnages et leurs etiquettes
QRectF Atelier3D::encombrement(const Camera &cam) const
{
    QVector<QVector3D> pts;
    const float hx = float(m_demiLargeur), hz = float(m_demiProfondeur);
    pts << V(-hx, 0, -hz) << V(hx, 0, -hz) << V(hx, 0, hz) << V(-hx, 0, hz)
        << V(-hx, -0.35f, hz) << V(hx, -0.35f, hz) << V(-hx, -0.35f, -hz) << V(hx, -0.35f, -hz);
    for (int i = 0; i < m_centres.size(); ++i)
        pts << V(m_centres[i].x(), 4.3f, m_centres[i].z() - 1.0f);
    qreal x0 = 1e9, y0 = 1e9, x1 = -1e9, y1 = -1e9;
    for (int i = 0; i < pts.size(); ++i) {
        QPointF e;
        if (!projeter(cam, pts[i], &e))
            return QRectF();
        x0 = qMin(x0, e.x()); y0 = qMin(y0, e.y()); x1 = qMax(x1, e.x()); y1 = qMax(y1, e.y());
    }
    return QRectF(QPointF(x0, y0), QPointF(x1, y1));
}

// distance de la camera pour laquelle toute la scene tient dans la zone (recherche par dichotomie)
qreal Atelier3D::distanceAjustee(qreal yaw, qreal pitch, const QRectF &zone) const
{
    const QRectF utile = zone.adjusted(14, 40, -14, -30);
    qreal bas = 6, haut = 220;
    for (int i = 0; i < 22; ++i) {
        const qreal milieu = (bas + haut) / 2;
        Camera cam;
        preparerCamera(cam, yaw, pitch, milieu, zone, false);
        const QRectF e = encombrement(cam);
        if (e.isValid() && e.width() <= utile.width() && e.height() <= utile.height())
            haut = milieu;
        else
            bas = milieu;
    }
    return haut;
}

// ------------------------------------------------------------------------------------
// Dessin
// ------------------------------------------------------------------------------------
void Atelier3D::dessinerFond(QPainter &p, const QRectF &zone) const
{
    QLinearGradient degrade(zone.topLeft(), zone.bottomLeft());
    degrade.setColorAt(0, QColor("#FCF8F1"));
    degrade.setColorAt(1, QColor("#F0E6D6"));
    p.fillRect(zone, degrade);
}

void Atelier3D::dessinerSolide(QPainter &p, const Solide &solide, int poste, qreal, qreal eclat)
{
    for (int f = 0; f < solide.faces.size(); ++f) {
        const Face &face = solide.faces[f];
        if (QVector3D::dotProduct(face.normale, m_camera.oeil - face.centre) <= 0)
            continue;                                   // face tournee de l'autre cote
        QPolygonF poly;
        bool ok = true;
        for (int i = 0; i < face.points.size(); ++i) {
            QPointF e;
            if (!projeter(m_camera, face.points[i], &e)) { ok = false; break; }
            poly << e;
        }
        if (!ok)
            continue;
        const qreal k = 0.56 + 0.52 * qMax<qreal>(0, QVector3D::dotProduct(face.normale, kLumiere)) + eclat;
        p.setBrush(nuance(solide.couleur, k));
        p.setPen(QPen(QColor(40, 25, 10, 60), 0.7));
        p.drawPolygon(poly);
        if (poste >= 0) {
            Zone z;
            z.poste = poste;
            z.polygone = poly;
            m_zones.append(z);
        }
    }
}

void Atelier3D::dessinerSol(QPainter &p)
{
    const float hx = float(m_demiLargeur), hz = float(m_demiProfondeur);
    // dalle : tranche visible + dessus
    dessinerSolide(p, boite(0, 0, 2 * hx, 2 * hz, -0.35f, 0, QColor("#CDBFA8")), -1, 1, 0);
    // carrelage en damier (carreaux de 2 unites ; trait de meme couleur pour cacher les raccords)
    const int nx = int(std::ceil(hx)), nz = int(std::ceil(hz));
    for (int ix = 0; ix < nx; ++ix) {
        for (int iz = 0; iz < nz; ++iz) {
            const float x0 = -hx + ix * 2.0f, x1 = qMin(hx, x0 + 2.0f);
            const float z0 = -hz + iz * 2.0f, z1 = qMin(hz, z0 + 2.0f);
            if (x1 <= x0 || z1 <= z0)
                break;
            QPointF a, b, c, d;
            if (!projeter(m_camera, V(x0, 0, z0), &a) || !projeter(m_camera, V(x1, 0, z0), &b)
                || !projeter(m_camera, V(x1, 0, z1), &c) || !projeter(m_camera, V(x0, 0, z1), &d))
                continue;
            const QColor couleur = ((ix + iz) % 2 == 0) ? QColor("#EEE5D6") : QColor("#E7DBC8");
            p.setBrush(couleur);
            p.setPen(QPen(couleur, 0.8));
            QPolygonF q;
            q << a << b << c << d;
            p.drawPolygon(q);
        }
    }

    for (int i = 0; i < m_postes.size(); ++i) {
        const QVector3D c = m_centres[i];
        const Etat &etat = m_etats[i];
        p.setOpacity(etat.visibilite);
        // ombre portee douce autour de l'ilot
        QPointF a, b, cc, d;
        const float ox = 0.35f, oz = 0.3f, mx = kLargeurPad / 2 + 0.25f, mz = kProfondeurPad / 2 + 0.25f;
        if (projeter(m_camera, V(c.x() - mx + ox, 0.01f, c.z() - mz + oz), &a)
            && projeter(m_camera, V(c.x() + mx + ox, 0.01f, c.z() - mz + oz), &b)
            && projeter(m_camera, V(c.x() + mx + ox, 0.01f, c.z() + mz + oz), &cc)
            && projeter(m_camera, V(c.x() - mx + ox, 0.01f, c.z() + mz + oz), &d)) {
            QPolygonF ombre;
            ombre << a << b << cc << d;
            p.setBrush(QColor(70, 50, 30, 34));
            p.setPen(Qt::NoPen);
            p.drawPolygon(ombre);
        }
        // tapis du poste : vert = libre, bordeaux = occupe, marine = mon poste
        const bool occupe = m_occupants.contains(m_postes[i]);
        const bool moi = !m_monPoste.isEmpty() && m_postes[i] == m_monPoste;
        QColor tapis = moi ? QColor("#C5D6E2") : (occupe ? QColor("#EBC9CE") : QColor("#CFE3D3"));
        const qreal respiration = occupe ? 0 : 0.035 * std::sin(m_temps * 2.4 + i);
        dessinerSolide(p, boite(c.x(), c.z(), kLargeurPad, kProfondeurPad, 0, kEpaisseurPad, tapis), i, 1,
                       etat.survol * 0.10 + respiration);
        // ombre de l'employe sur le tapis
        if (etat.presence > 0.02) {
            QPolygonF tache;
            bool ok = true;
            for (int k = 0; k < 14 && ok; ++k) {
                const double angle = 2 * kPi * k / 14;
                QPointF e;
                ok = projeter(m_camera, V(c.x() + 0.2f + float(0.95 * std::cos(angle)), kEpaisseurPad + 0.01f,
                                           c.z() - 1.45f + float(0.55 * std::sin(angle))), &e);
                tache << e;
            }
            if (ok) {
                p.setBrush(QColor(60, 40, 20, int(70 * etat.presence)));
                p.setPen(Qt::NoPen);
                p.drawPolygon(tache);
            }
        }
        p.setOpacity(1);
    }

    // anneau dore autour du poste choisi
    if (m_selection >= 0 && m_selection < m_postes.size()) {
        const QVector3D c = m_centres[m_selection];
        const float mx = kLargeurPad / 2 + 0.12f, mz = kProfondeurPad / 2 + 0.12f, y = kEpaisseurPad + 0.02f;
        QPointF a, b, cc, d;
        if (projeter(m_camera, V(c.x() - mx, y, c.z() - mz), &a) && projeter(m_camera, V(c.x() + mx, y, c.z() - mz), &b)
            && projeter(m_camera, V(c.x() + mx, y, c.z() + mz), &cc) && projeter(m_camera, V(c.x() - mx, y, c.z() + mz), &d)) {
            QPolygonF anneau;
            anneau << a << b << cc << d;
            const int alpha = 190 + int(55 * std::sin(m_temps * 4.0));
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(QColor(214, 165, 60, qBound(120, alpha, 255)), 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.drawPolygon(anneau);
        }
    }
}

void Atelier3D::dessinerEtiquettes(QPainter &p)
{
    const int n = m_postes.size();
    m_rectEtiquettes = QVector<QRectF>(n);

    // du plus proche au plus lointain : le plus proche garde sa place, les autres montent si besoin
    struct Ordre { float profondeur; int index; };
    QVector<Ordre> ordre;
    for (int i = 0; i < n; ++i) {
        Ordre o;
        o.profondeur = (m_camera.oeil - m_centres[i]).length();
        o.index = i;
        ordre.append(o);
    }
    std::sort(ordre.begin(), ordre.end(), [](const Ordre &a, const Ordre &b) { return a.profondeur < b.profondeur; });

    // les etiquettes grandissent un peu avec la zone (grand ecran), sans devenir enormes
    const qreal e = qBound<qreal>(1.0, m_camera.zone.height() / 470.0, 1.45);
    QFont gras = p.font();
    gras.setPointSizeF(8.5 * e);
    gras.setBold(true);
    QFont normal = p.font();
    normal.setPointSizeF(8.5 * e);
    normal.setBold(false);
    const QFontMetricsF mg(gras), mn(normal);
    const qreal hauteur = 22 * e;

    struct Pose { QRectF rect; QPointF ancre; QString nom, suite; };
    QVector<Pose> poses(n);
    QVector<QRectF> deja;
    for (int k = 0; k < ordre.size(); ++k) {
        const int i = ordre[k].index;
        const QVector3D c = m_centres[i];
        QPointF ancre;
        const bool occupe = m_occupants.contains(m_postes[i]);
        const bool moi = !m_monPoste.isEmpty() && m_postes[i] == m_monPoste;
        const float haut = occupe ? 4.35f : 3.2f;       // un poste vide : l'etiquette descend vers le tapis
        if (!projeter(m_camera, V(c.x(), haut, c.z() - 1.0f), &ancre))
            continue;
        Pose &po = poses[i];
        po.nom = PosteTravail::libelleCourt(m_postes[i]);
        po.suite = moi ? QString::fromUtf8("Vous") : (occupe ? initialesCourtes(m_occupants.value(m_postes[i])) : QString());
        po.suite = mn.elidedText(po.suite, Qt::ElideRight, 120);
        qreal l = 26 * e + mg.horizontalAdvance(po.nom);
        if (!po.suite.isEmpty())
            l += 10 * e + mn.horizontalAdvance(po.suite);
        QRectF rect(ancre.x() - l / 2, ancre.y() - hauteur - 8 * e, l, hauteur);
        rect.moveLeft(qBound(m_camera.zone.left() + 4, rect.left(), qMax(m_camera.zone.left() + 4, m_camera.zone.right() - 4 - l)));
        for (int essai = 0; essai < 12; ++essai) {      // pas de chevauchement entre etiquettes
            bool gene = false;
            for (int d = 0; d < deja.size(); ++d) {
                if (deja[d].adjusted(-3, -2, 3, 2).intersects(rect)) {
                    rect.moveBottom(deja[d].top() - 3);
                    gene = true;
                    break;
                }
            }
            if (!gene)
                break;
        }
        rect.moveTop(qMax(m_camera.zone.top() + 4, rect.top()));
        deja.append(rect);
        po.rect = rect;
        po.ancre = ancre;
        m_rectEtiquettes[i] = rect;
    }

    for (int k = ordre.size() - 1; k >= 0; --k) {      // on dessine du plus lointain au plus proche
        const int i = ordre[k].index;
        const Pose &po = poses[i];
        if (po.rect.isEmpty())
            continue;
        const bool occupe = m_occupants.contains(m_postes[i]);
        const bool moi = !m_monPoste.isEmpty() && m_postes[i] == m_monPoste;
        const bool choisi = (i == m_selection);
        const QColor accent = couleurAccent(i);
        p.setOpacity(m_etats[i].visibilite);

        QPainterPath fond;
        fond.addRoundedRect(po.rect, hauteur / 2, hauteur / 2);
        const qreal px = qBound(po.rect.left() + 12 * e, po.ancre.x(), po.rect.right() - 12 * e);
        QPolygonF pointe;
        pointe << QPointF(px - 4.5 * e, po.rect.bottom() - 0.5) << QPointF(px + 4.5 * e, po.rect.bottom() - 0.5)
               << QPointF(px, po.rect.bottom() + 5.5 * e);
        QPainterPath pt;
        pt.addPolygon(pointe);
        fond = fond.united(pt);
        p.setBrush(QColor(255, 253, 249, 244));
        p.setPen(QPen(choisi ? QColor(214, 165, 60) : accent, choisi ? 2.4 : 1.4));
        p.drawPath(fond);

        p.setBrush(accent);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(po.rect.left() + 12 * e, po.rect.center().y()), 3.6 * e, 3.6 * e);
        p.setFont(gras);
        p.setPen(QColor(kMarine));
        const qreal xTexte = po.rect.left() + 21 * e;
        const qreal yTexte = po.rect.center().y() + mg.ascent() / 2 - 0.5;
        p.drawText(QPointF(xTexte, yTexte), po.nom);
        if (!po.suite.isEmpty()) {
            p.setFont(normal);
            p.setPen(QColor(moi ? kMarine : (occupe ? kBordeaux : kGris)));
            p.drawText(QPointF(xTexte + mg.horizontalAdvance(po.nom) + 8 * e, yTexte), po.suite);
        }
        p.setOpacity(1);
    }
}

void Atelier3D::dessinerLegende(QPainter &p, const QRectF &zone) const
{
    QFont f = p.font();
    f.setPointSizeF(8);
    f.setBold(false);
    p.setFont(f);
    const QFontMetricsF fm(f);
    struct Entree { QColor c; QString t; };
    QVector<Entree> entrees;
    Entree libre; libre.c = QColor(kVert); libre.t = "Libre";
    Entree occupe; occupe.c = QColor(kBordeaux); occupe.t = QString::fromUtf8("Occupé");
    entrees << libre << occupe;
    if (m_actions) {
        Entree moi; moi.c = QColor(kMarine); moi.t = "Mon poste";
        entrees << moi;
    }
    qreal x = zone.left() + 14;
    const qreal y = zone.bottom() - 16;
    for (int i = 0; i < entrees.size(); ++i) {
        p.setBrush(entrees[i].c);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(x + 4, y), 4, 4);
        p.setPen(QColor(kGris));
        p.drawText(QPointF(x + 14, y + 4), entrees[i].t);
        x += 14 + fm.horizontalAdvance(entrees[i].t) + 14;
    }
}

void Atelier3D::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    const QRectF zone = zoneScene();

    // cadre arrondi de la scene
    QPainterPath cadre;
    cadre.addRoundedRect(zone.adjusted(0.5, 0.5, -0.5, -0.5), 12, 12);
    p.setClipPath(cadre);
    dessinerFond(p, zone);

    if (m_postes.isEmpty()) {
        p.setPen(QColor(kGris));
        p.drawText(zone, Qt::AlignCenter, QString::fromUtf8("Aucun poste à afficher"));
        p.setClipping(false);
        return;
    }

    preparerCamera(m_camera, m_yaw + m_balayage, m_pitch, m_distance, zone, true);
    m_zones.clear();

    dessinerSol(p);

    // personnages (recalcules a chaque image : apparition en fondu et petit balancement)
    m_dyn.clear();
    QVector<int> ownerSolide;       // poste de chaque solide de m_dyn
    for (int i = 0; i < m_postes.size(); ++i) {
        const Etat &e = m_etats[i];
        if (e.presence < 0.02)
            continue;
        const bool moi = !m_monPoste.isEmpty() && m_postes[i] == m_monPoste;
        const QColor tenue = moi ? QColor(kMarine) : QColor(kBordeaux);
        const QVector<Solide> pers = personnage(i, m_centres[i], (1 - e.presence) * 0.7,
                                                0.035 * std::sin(m_temps * 2.2 + i * 1.7), tenue);
        for (int k = 0; k < pers.size(); ++k) {
            m_dyn.append(pers[k]);
            ownerSolide.append(i);
        }
    }

    QVector<Element> elements;
    for (int i = 0; i < m_decor.size(); ++i) {
        for (int k = 0; k < m_decor[i].size(); ++k) {
            Element el;
            el.profondeur = (m_camera.oeil - m_decor[i][k].centre).length();
            el.solide = &m_decor[i][k];
            el.poste = i;
            el.alpha = float(m_etats[i].visibilite);
            elements.append(el);
        }
    }
    for (int k = 0; k < m_dyn.size(); ++k) {
        Element el;
        el.profondeur = (m_camera.oeil - m_dyn[k].centre).length();
        el.solide = &m_dyn[k];
        el.poste = ownerSolide[k];
        el.alpha = float(m_etats[el.poste].visibilite * m_etats[el.poste].presence);
        elements.append(el);
    }
    for (int i = 0; i < m_postes.size(); ++i) {
        const Etat &e = m_etats[i];
        if (e.presence < 0.02)
            continue;
        Element el;
        el.tete = V(m_centres[i].x(), float((1 - e.presence) * 0.7 + 0.035 * std::sin(m_temps * 2.2 + i * 1.7)) + 3.3f,
                    m_centres[i].z() - 1.6f);
        el.profondeur = (m_camera.oeil - el.tete).length();
        el.solide = nullptr;
        el.poste = i;
        el.alpha = float(e.visibilite * e.presence);
        const Employe emp = m_occupants.value(m_postes[i]);
        const QColor cheveux[4] = {QColor("#2B2320"), QColor("#5A3A25"), QColor("#8A5A2B"), QColor("#3A2A22")};
        el.cheveux = cheveux[qAbs(emp.id()) % 4];
        elements.append(el);
    }
    std::sort(elements.begin(), elements.end(), [](const Element &a, const Element &b) { return a.profondeur > b.profondeur; });

    for (int k = 0; k < elements.size(); ++k) {
        const Element &el = elements[k];
        const Etat &etat = m_etats[el.poste];
        p.setOpacity(el.alpha);
        if (el.solide) {
            dessinerSolide(p, *el.solide, el.poste, 1, etat.survol * 0.10 + (el.poste == m_selection ? 0.05 : 0));
        } else {
            // tete : sphere simulee par un disque ombre, avec la coupe de cheveux
            QPointF centre, bord;
            if (projeter(m_camera, el.tete, &centre) && projeter(m_camera, el.tete + m_camera.droite * 0.42f, &bord)) {
                const qreal r = std::hypot(bord.x() - centre.x(), bord.y() - centre.y());
                QRadialGradient g(centre + QPointF(-r * 0.3, -r * 0.35), r * 1.5);
                g.setColorAt(0, QColor("#F2D3B3"));
                g.setColorAt(1, QColor("#C99A72"));
                p.setBrush(g);
                p.setPen(QPen(QColor(40, 25, 10, 70), 0.8));
                p.drawEllipse(centre, r, r);
                p.setBrush(el.cheveux);
                p.setPen(Qt::NoPen);
                p.drawChord(QRectF(centre.x() - r, centre.y() - r, 2 * r, 2 * r), 10 * 16, 160 * 16);
            }
        }
    }
    p.setOpacity(1);

    dessinerEtiquettes(p);
    dessinerLegende(p, zone);

    // aide, a gauche du bouton "Vue d'ensemble"
    QFont f = p.font();
    f.setPointSizeF(8);
    p.setFont(f);
    p.setPen(QColor(kGris));
    p.drawText(QRectF(zone.left() + 150, zone.bottom() - 28, zone.width() - 150 - m_boutonVue->width() - 24, 24),
               Qt::AlignRight | Qt::AlignVCenter, QString::fromUtf8("Glisser : pivoter  ·  Molette : zoom"));

    p.setClipping(false);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor("#EADFD0"), 1));
    p.drawRoundedRect(zone.adjusted(0.5, 0.5, -0.5, -0.5), 12, 12);
}

// ------------------------------------------------------------------------------------
// Animation
// ------------------------------------------------------------------------------------
void Atelier3D::surTic()
{
    qreal dt = m_horloge.restart() / 1000.0;
    dt = qBound<qreal>(0.001, dt, 0.1);
    m_temps += dt;
    const qreal facile = qMin<qreal>(1, dt * 9);

    m_yaw += (m_yawCible - m_yaw) * facile;
    m_pitch += (m_pitchCible - m_pitch) * facile;
    m_zoom += (m_zoomCible - m_zoom) * facile;

    // quand personne ne touche a la scene, elle oscille doucement pour donner du relief
    const bool calme = !m_appui && m_depuisAction.elapsed() > 6000;
    m_balayageForce += ((calme ? 1.0 : 0.0) - m_balayageForce) * qMin<qreal>(1, dt * 1.4);
    m_balayage = m_balayageForce * 14.0 * std::sin(m_temps * 0.5);

    const QRectF zone = zoneScene();
    const qreal cible = distanceAjustee(m_yaw + m_balayage, m_pitch, zone) / m_zoom;
    m_distance += (cible - m_distance) * qMin<qreal>(1, dt * 8);

    bool mouvement = m_appui || qAbs(m_yawCible - m_yaw) > 0.05 || qAbs(m_pitchCible - m_pitch) > 0.05
                     || qAbs(m_zoomCible - m_zoom) > 0.002 || qAbs(cible - m_distance) > 0.05;
    for (int i = 0; i < m_etats.size() && i < m_postes.size(); ++i) {
        Etat &e = m_etats[i];
        const bool occupe = m_occupants.contains(m_postes[i]);
        e.presence += ((occupe ? 1.0 : 0.0) - e.presence) * qMin<qreal>(1, dt * 3.5);
        if (qAbs((occupe ? 1.0 : 0.0) - e.presence) < 0.004)
            e.presence = occupe ? 1.0 : 0.0;
        const bool visible = m_filtre == 0 || (m_filtre == 1 && occupe) || (m_filtre == 2 && !occupe);
        e.visibilite += ((visible ? 1.0 : 0.28) - e.visibilite) * qMin<qreal>(1, dt * 6);
        e.survol += ((i == m_survolePoste ? 1.0 : 0.0) - e.survol) * qMin<qreal>(1, dt * 10);
        if (qAbs((occupe ? 1.0 : 0.0) - e.presence) > 0.004 || qAbs((visible ? 1.0 : 0.28) - e.visibilite) > 0.01
            || qAbs((i == m_survolePoste ? 1.0 : 0.0) - e.survol) > 0.01)
            mouvement = true;
    }
    // plein regime pendant un mouvement ; au repos, seules les pulsations douces tournent (moins de calcul)
    const int intervalle = mouvement ? 33 : 66;
    if (m_minuteur->interval() != intervalle)
        m_minuteur->setInterval(intervalle);
    update();
}

void Atelier3D::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    m_horloge.restart();
    m_minuteur->start();
}

void Atelier3D::hideEvent(QHideEvent *e)
{
    QWidget::hideEvent(e);
    m_minuteur->stop();
}

void Atelier3D::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    const QRectF zone = zoneScene();
    m_boutonVue->adjustSize();
    m_boutonVue->move(int(zone.right()) - m_boutonVue->width() - 10, int(zone.bottom()) - m_boutonVue->height() - 8);
    if (!m_postes.isEmpty())
        m_distance = distanceAjustee(m_yaw + m_balayage, m_pitch, zone) / m_zoom;   // pas de glissement au redimensionnement
}

// ------------------------------------------------------------------------------------
// Souris
// ------------------------------------------------------------------------------------
int Atelier3D::posteSous(const QPointF &position) const
{
    for (int i = m_rectEtiquettes.size() - 1; i >= 0; --i)
        if (m_rectEtiquettes[i].contains(position) && m_etats.value(i).visibilite > 0.5)
            return i;
    for (int k = m_zones.size() - 1; k >= 0; --k)
        if (m_zones[k].polygone.containsPoint(position, Qt::OddEvenFill) && m_etats.value(m_zones[k].poste).visibilite > 0.5)
            return m_zones[k].poste;
    return -1;
}

void Atelier3D::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton || !zoneScene().contains(positionDe(e)))
        return;
    m_appui = true;
    m_glisse = false;
    m_dernierePosition = positionDe(e).toPoint();
    m_positionAppui = m_dernierePosition;
    m_depuisAction.restart();
}

void Atelier3D::mouseMoveEvent(QMouseEvent *e)
{
    const QPoint pos = positionDe(e).toPoint();
    if (m_appui) {
        if (!m_glisse && (pos - m_positionAppui).manhattanLength() > 4) {
            m_glisse = true;
            setCursor(Qt::ClosedHandCursor);
        }
        if (m_glisse) {
            m_yawCible -= (pos.x() - m_dernierePosition.x()) * 0.45;
            m_pitchCible = qBound<qreal>(18, m_pitchCible + (pos.y() - m_dernierePosition.y()) * 0.3, 72);
        }
        m_dernierePosition = pos;
        m_depuisAction.restart();
        return;
    }
    const int sous = zoneScene().contains(positionDe(e)) ? posteSous(positionDe(e)) : -1;
    if (sous != m_survolePoste) {
        m_survolePoste = sous;
        setCursor(sous >= 0 ? Qt::PointingHandCursor : Qt::OpenHandCursor);
        if (m_selection < 0)
            majBandeau();
    }
}

void Atelier3D::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton || !m_appui)
        return;
    m_appui = false;
    setCursor(m_survolePoste >= 0 ? Qt::PointingHandCursor : Qt::OpenHandCursor);
    if (!m_glisse) {                                    // simple clic : choisir (ou lever la selection)
        const int sous = posteSous(positionDe(e));
        m_selection = sous;
        majBandeau();
        emit posteChoisi(posteSelectionne());
    }
    m_glisse = false;
    m_depuisAction.restart();
}

void Atelier3D::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && zoneScene().contains(positionDe(e)) && posteSous(positionDe(e)) < 0)
        reinitialiserVue();
}

void Atelier3D::wheelEvent(QWheelEvent *e)
{
    if (!zoneScene().contains(positionDe(e))) {
        e->ignore();
        return;
    }
    m_zoomCible = qBound<qreal>(0.7, m_zoomCible * std::pow(1.0016, e->angleDelta().y()), 2.4);
    m_depuisAction.restart();
    e->accept();
}

void Atelier3D::leaveEvent(QEvent *e)
{
    QWidget::leaveEvent(e);
    if (m_survolePoste != -1) {
        m_survolePoste = -1;
        if (m_selection < 0)
            majBandeau();
    }
}

// ------------------------------------------------------------------------------------
// Bandeau d'information
// ------------------------------------------------------------------------------------
void Atelier3D::majBandeau()
{
    const int i = m_selection >= 0 ? m_selection : m_survolePoste;
    if (i < 0 || i >= m_postes.size()) {
        m_pastille->setPixmap(pixmapIcone(Icone::Groupe, QColor(kMarine), 26));
        m_pastille->setStyleSheet("background-color: #F6EDE4; border-radius: 21px;");
        m_titre->setText("Plan 3D de l'atelier");
        m_detail->setText(QString::fromUtf8("Faites pivoter la scène à la souris, puis cliquez sur un poste pour voir qui l'occupe."));
        m_badge->hide();
        m_action->hide();
        return;
    }

    const QString poste = m_postes[i];
    const bool occupe = m_occupants.contains(poste);
    const bool moi = !m_monPoste.isEmpty() && poste == m_monPoste;
    const QColor accent = couleurAccent(i);
    const Employe e = m_occupants.value(poste);

    m_pastille->setPixmap(pixmapPictogrammePoste(PosteTravail::type(poste), accent, 26));
    m_pastille->setStyleSheet(QString("background-color: %1; border-radius: 21px;").arg(occupe ? "#F6EDE4" : "#E8F1E9"));
    m_titre->setText(PosteTravail::libelle(poste));
    if (occupe) {
        QString detail = e.prenom() + " " + e.nom() + QString::fromUtf8(" · ") + e.poste();
        const QString service = m_services.value(e.idService());
        if (!service.isEmpty())
            detail += QString::fromUtf8(" · Service : ") + service;
        if (moi)
            detail += QString::fromUtf8(" · vous");
        m_detail->setText(detail);
    } else {
        m_detail->setText(QString::fromUtf8("Aucun employé n'occupe ce poste."));
    }
    m_badge->setText(occupe ? QString::fromUtf8("●  Occupé") : QString::fromUtf8("●  Libre"));
    m_badge->setStyleSheet(QString("background-color: %1; color: white; border-radius: 11px; padding: 0px 14px; font-size: 9pt; font-weight: bold;")
                               .arg(occupe ? kBordeaux : kVert));
    m_badge->show();

    // espace employe : prendre un poste libre ou liberer le sien ; un poste occupe par un autre n'a pas de bouton
    if (m_actions && m_selection >= 0 && (moi || !occupe)) {
        m_action->setText(moi ? QString::fromUtf8("Libérer mon poste") : "Prendre ce poste");
        m_action->setStyleSheet(QString(
            "QPushButton { background-color: %1; color: white; border: none; border-radius: 6px; padding: 6px 14px; font-weight: bold; }"
            "QPushButton:hover { background-color: %2; }").arg(moi ? kBordeaux : kMarine, moi ? "#6E1725" : "#2B4F68"));
        m_action->show();
    } else {
        m_action->hide();
    }
}
