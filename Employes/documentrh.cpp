#include "documentrh.h"
#include "pictogrammes.h"

#include <QFile>
#include <QFontMetricsF>
#include <QLocale>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPainterPath>
#include <QPdfWriter>
#include <QPixmap>
#include <QPolygonF>

namespace {

// Le document est dessine dans un repere de 1000 x 1414 unites (proportions A4), mis a
// l'echelle de la page au moment du dessin.
constexpr qreal kLargeur = 1000;
constexpr qreal kHauteur = 1414;

const QColor kMarine("#14304A");
const QColor kBordeaux("#8B1E2F");
const QColor kOr("#B8975A");
const QColor kOrClair("#E6D2B5");
const QColor kFondCellule("#F4EEE6");
const QColor kGris("#5B6670");

QString dateLongue(const QDate &date)
{
    return QLocale(QLocale::French).toString(date, "dddd d MMMM yyyy");
}

QFont police(const QString &famille, int taillePx, bool gras = false)
{
    QFont f(famille);
    f.setPixelSize(taillePx);
    f.setBold(gras);
    return f;
}

void dessinerEtoile(QPainter &p, const QPointF &centre, qreal rayon)
{
    const qreal creux = rayon * 0.28;
    QPolygonF etoile;
    etoile << QPointF(centre.x(), centre.y() - rayon)
           << QPointF(centre.x() + creux, centre.y() - creux)
           << QPointF(centre.x() + rayon, centre.y())
           << QPointF(centre.x() + creux, centre.y() + creux)
           << QPointF(centre.x(), centre.y() + rayon)
           << QPointF(centre.x() - creux, centre.y() + creux)
           << QPointF(centre.x() - rayon, centre.y())
           << QPointF(centre.x() - creux, centre.y() - creux);
    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(kOr);
    p.drawPolygon(etoile);
    p.restore();
}

// Ruban bordeaux a pointe + cadre arrondi contenant 4 lignes "icone | libelle | valeur".
struct Ligne {
    Icone icone;
    QString libelle;
    QString valeur;
};

void dessinerSection(QPainter &p, const QString &titre, qreal yRuban, const QList<Ligne> &lignes)
{
    // cadre
    const QRectF cadre(33, yRuban + 28, 934, 328);
    p.setPen(QPen(kOrClair, 1.5));
    p.setBrush(QColor("#FBF7F2"));
    p.drawRoundedRect(cadre, 16, 16);

    // ruban
    p.setPen(Qt::NoPen);
    p.setBrush(kBordeaux);
    p.drawRoundedRect(QRectF(45, yRuban, 300, 56), 12, 12);
    p.drawPolygon(QPolygonF({QPointF(330, yRuban), QPointF(362, yRuban + 28), QPointF(330, yRuban + 56)}));
    p.setPen(Qt::white);
    p.setFont(police("Playfair Display", 29, true));
    p.drawText(QRectF(72, yRuban, 260, 56), Qt::AlignLeft | Qt::AlignVCenter, titre);

    // lignes
    qreal y = cadre.top() + 44;
    for (const Ligne &ligne : lignes) {
        const QRectF celluleLibelle(55, y, 285, 58);
        p.setPen(Qt::NoPen);
        p.setBrush(kFondCellule);
        p.drawRoundedRect(celluleLibelle, 8, 8);
        dessinerIcone(p, ligne.icone, QRectF(70, y + 12, 34, 34), kBordeaux);
        p.setPen(kMarine);
        p.setFont(police("Playfair Display", 24, true));
        p.drawText(QRectF(125, y, 210, 58), Qt::AlignLeft | Qt::AlignVCenter, ligne.libelle);

        const QRectF celluleValeur(351, y, 616, 58);
        p.setPen(QPen(kOrClair, 1.5));
        p.setBrush(Qt::white);
        p.drawRoundedRect(celluleValeur, 8, 8);
        p.setPen(kMarine);
        const QFont fonteValeur = police("Playfair Display", 27);
        p.setFont(fonteValeur);
        p.drawText(QRectF(376, y, 580, 58), Qt::AlignLeft | Qt::AlignVCenter,
                   QFontMetricsF(fonteValeur).elidedText(ligne.valeur, Qt::ElideRight, 570));
        y += 68;
    }
}

} // namespace

void DocumentRH::dessinerDemandeConge(QPainter &p, const QRectF &page, const Employe &employe,
                                      const QDate &debut, const QDate &fin, const QString &motif,
                                      const QDate &dateDemande)
{
    p.save();
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    p.translate(page.topLeft());
    p.scale(page.width() / kLargeur, page.height() / kHauteur);

    p.fillRect(QRectF(0, 0, kLargeur, kHauteur), QColor("#FDFBF8"));

    // --- en-tete : logo, filet dore avec etoile, nom de l'atelier ---
    const QPixmap logo(":/images/logo_smartstitch.png");
    if (!logo.isNull()) {
        const qreal hauteur = 86;
        const qreal largeur = hauteur * logo.width() / logo.height();
        p.drawPixmap(QRectF(50, 38, largeur, hauteur), logo, logo.rect());
    }
    p.setPen(QPen(kOr, 1.4));
    p.drawLine(QPointF(335, 100), QPointF(690, 100));
    dessinerEtoile(p, QPointF(712, 100), 11);
    p.setPen(QPen(kOr, 1.4));
    p.drawLine(QPointF(733, 100), QPointF(955, 100));

    QFont fonteAtelier = police("Lato", 16, true);
    fonteAtelier.setLetterSpacing(QFont::AbsoluteSpacing, 3);
    p.setFont(fonteAtelier);
    p.setPen(kMarine);
    p.drawText(QRectF(560, 56, 395, 22), Qt::AlignRight | Qt::AlignVCenter, QString::fromUtf8("ATELIER VINTAGE COUTURE"));
    QFont fonteDevise = police("Lato", 12, true);
    fonteDevise.setLetterSpacing(QFont::AbsoluteSpacing, 3);
    p.setFont(fonteDevise);
    p.setPen(kOr);
    p.drawText(QRectF(560, 78, 395, 16), Qt::AlignRight | Qt::AlignVCenter, QString::fromUtf8("ÉLÉGANCE & TRADITION"));

    // --- titre : "Demande de" marine, "congé" bordeaux ---
    const QFont fonteTitre = police("Playfair Display", 70, true);
    const QFontMetricsF fm(fonteTitre);
    const QString debutTitre = "Demande de ";
    const QString finTitre = QString::fromUtf8("congé");
    const qreal largeurTitre = fm.horizontalAdvance(debutTitre) + fm.horizontalAdvance(finTitre);
    const qreal xTitre = (kLargeur - largeurTitre) / 2;
    const qreal yBase = 235;
    p.setFont(fonteTitre);
    p.setPen(kMarine);
    p.drawText(QPointF(xTitre, yBase), debutTitre);
    p.setPen(kBordeaux);
    p.drawText(QPointF(xTitre + fm.horizontalAdvance(debutTitre), yBase), finTitre);

    p.setPen(QPen(kOr, 1.4));
    p.drawLine(QPointF(385, 268), QPointF(480, 268));
    dessinerEtoile(p, QPointF(500, 268), 10);
    p.setPen(QPen(kOr, 1.4));
    p.drawLine(QPointF(520, 268), QPointF(615, 268));

    p.setFont(police("Lato", 24));
    p.setPen(kGris);
    p.drawText(QRectF(0, 285, kLargeur, 34), Qt::AlignHCenter | Qt::AlignVCenter,
               QString::fromUtf8("Établi le ") + dateLongue(dateDemande));

    // --- sections ---
    const int nombreJours = int(debut.daysTo(fin)) + 1;
    const QString motifAffiche = motif.trimmed().isEmpty() ? QString::fromUtf8("Non précisé") : motif.trimmed();

    dessinerSection(p, QString::fromUtf8("Employé"), 350, {
        {Icone::Personne, "Nom et prénom", employe.prenom() + " " + employe.nom()},
        {Icone::Mallette, "Poste", employe.poste()},
        {Icone::Enveloppe, "Email", employe.email()},
        {Icone::Telephone, QString::fromUtf8("Téléphone"), employe.telephone()},
    });
    dessinerSection(p, QString::fromUtf8("Période demandée"), 740, {
        {Icone::Calendrier, "Du", dateLongue(debut)},
        {Icone::Calendrier, "Au", dateLongue(fin)},
        {Icone::Horloge, QString::fromUtf8("Durée"), QString::fromUtf8("%1 jour(s) calendaire(s)").arg(nombreJours)},
        {Icone::Document, "Motif", motifAffiche},
    });

    // --- signatures ---
    p.setFont(police("Playfair Display", 26));
    p.setPen(kMarine);
    p.drawText(QRectF(45, 1135, 445, 36), Qt::AlignHCenter | Qt::AlignVCenter, QString::fromUtf8("Signature de l'employé"));
    p.drawText(QRectF(533, 1135, 447, 36), Qt::AlignHCenter | Qt::AlignVCenter, QString::fromUtf8("Avis et signature du gérant"));
    p.setPen(QPen(kOr, 1.6));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(45, 1180, 445, 150), 14, 14);
    p.drawRoundedRect(QRectF(533, 1180, 447, 150), 14, 14);

    // --- pied de page ---
    p.setFont(police("Lato", 14));
    p.setPen(kGris);
    p.drawText(QRectF(0, 1362, kLargeur, 22), Qt::AlignHCenter | Qt::AlignVCenter,
               QString::fromUtf8("SmartStitch — Atelier Vintage Couture · Fashion Management System"));

    p.restore();
}

bool DocumentRH::exporterDemandeConge(const Employe &employe, const QDate &debut, const QDate &fin,
                                      const QString &motif, const QString &chemin, QString *erreur)
{
    if (!debut.isValid() || !fin.isValid() || fin < debut) {
        if (erreur)
            *erreur = QString::fromUtf8("La date de fin doit être postérieure ou égale à la date de début.");
        return false;
    }

    QPdfWriter pdf(chemin);
    pdf.setPageSize(QPageSize(QPageSize::A4));
    pdf.setPageMargins(QMarginsF(0, 0, 0, 0));
    pdf.setResolution(150);
    pdf.setTitle(QString::fromUtf8("Demande de congé"));
    pdf.setCreator("SmartStitch");

    QPainter p(&pdf);
    if (!p.isActive()) {
        if (erreur)
            *erreur = "Impossible d'écrire le fichier : " + chemin;
        return false;
    }
    dessinerDemandeConge(p, QRectF(0, 0, pdf.width(), pdf.height()), employe, debut, fin, motif,
                         QDate::currentDate());
    p.end();

    if (!QFile::exists(chemin) || QFile(chemin).size() == 0) {
        if (erreur)
            *erreur = "Impossible d'écrire le fichier : " + chemin;
        return false;
    }
    return true;
}
