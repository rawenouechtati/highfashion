#include "repartitionpostes.h"
#include "pictogrammes.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>

namespace {

const QColor kMarine("#1F3A4D");
const QColor kGris("#8A8375");

// une couleur par segment, dans l'ordre
const QList<QColor> &couleursSegments()
{
    static const QList<QColor> couleurs = {
        QColor("#1F3A4D"), QColor("#8B1E2F"), QColor("#C9A66B"), QColor("#4F7857"), QColor("#D9CDB6")};
    return couleurs;
}

} // namespace

RepartitionPostes::RepartitionPostes(QWidget *parent)
    : QWidget(parent)
{
}

void RepartitionPostes::setDonnees(const QList<QPair<QString, int>> &parts)
{
    m_parts = parts;
    update();
}

void RepartitionPostes::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    int total = 0;
    for (const auto &part : std::as_const(m_parts))
        total += part.second;

    // --- une seule ligne d'en-tete : pastille + titre a gauche, nombre d'employes a droite ---
    const qreal marge = 4;
    QFont fonteTitre("Playfair Display");
    fonteTitre.setPointSizeF(12);
    fonteTitre.setBold(true);
    p.setFont(fonteTitre);
    p.setPen(kMarine);
    p.drawText(QRectF(marge, 0, width() - 150, 34), Qt::AlignLeft | Qt::AlignVCenter,
               QString::fromUtf8("Répartition par poste"));

    QFont fonteSous("Lato");
    fonteSous.setPointSizeF(9);
    p.setFont(fonteSous);
    p.setPen(kGris);
    p.drawText(QRectF(width() - 130, 0, 126, 34), Qt::AlignRight | Qt::AlignVCenter,
               total > 1 ? QString::fromUtf8("%1 employés").arg(total) : QString::fromUtf8("%1 employé").arg(total));

    // --- barre : segments proportionnels, bouts arrondis ---
    const QRectF barre(marge, 44, width() - 2 * marge, 12);
    QPainterPath forme;
    forme.addRoundedRect(barre, 6, 6);
    p.save();
    p.setClipPath(forme);
    p.fillRect(barre, QColor("#EADFD0"));          // fond (visible s'il n'y a aucun employe)
    qreal x = barre.left();
    int couleur = 0;
    for (const auto &part : std::as_const(m_parts)) {
        if (part.second > 0 && total > 0) {
            const qreal largeur = barre.width() * part.second / total;
            p.fillRect(QRectF(x, barre.top(), largeur + 0.5, barre.height()), couleursSegments().at(couleur % couleursSegments().size()));
            x += largeur;
        }
        ++couleur;
    }
    p.restore();

    // --- legende : pastille de couleur + "Poste N", a la ligne si la place manque ---
    QFont fonteLegende("Lato");
    fonteLegende.setPointSizeF(9.5);
    p.setFont(fonteLegende);
    const QFontMetricsF fm(fonteLegende);
    qreal xl = marge;
    qreal yl = 64;
    couleur = 0;
    for (const auto &part : std::as_const(m_parts)) {
        const QString texte = QString("%1 %2").arg(part.first).arg(part.second);
        const qreal largeurTexte = fm.horizontalAdvance(texte);
        if (xl + 20 + largeurTexte > width() - marge && xl > marge) {
            xl = marge;
            yl += 20;
        }
        p.setPen(Qt::NoPen);
        p.setBrush(couleursSegments().at(couleur % couleursSegments().size()));
        p.drawEllipse(QRectF(xl, yl + 4, 11, 11));
        p.setPen(kMarine);
        p.drawText(QRectF(xl + 17, yl, largeurTexte + 4, 19), Qt::AlignLeft | Qt::AlignVCenter, texte);
        xl += 17 + largeurTexte + 22;
        ++couleur;
    }
}
