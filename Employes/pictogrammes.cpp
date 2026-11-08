#include "pictogrammes.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QScreen>

namespace {

qreal ratioEcran()
{
    const QScreen *ecran = QGuiApplication::primaryScreen();
    return ecran ? qMax<qreal>(2.0, ecran->devicePixelRatio()) : 2.0;
}

QPixmap nouveauPixmap(int taille)
{
    const qreal ratio = ratioEcran();
    QPixmap pixmap(QSize(taille, taille) * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);
    return pixmap;
}

} // namespace

void dessinerPictogrammePoste(QPainter &p, const QString &type, const QColor &couleur)
{
    p.save();
    QPen stylo(couleur, 0.14, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(stylo);
    p.setBrush(Qt::NoBrush);

    if (type == "Couture") {                       // machine a coudre (corps en "C" ouvert a gauche)
        QPainterPath corps;
        corps.moveTo(-0.55, -0.62);
        corps.lineTo(0.72, -0.62);
        corps.lineTo(0.72, 0.8);
        corps.lineTo(-0.92, 0.8);
        corps.lineTo(-0.92, 0.5);
        corps.lineTo(0.42, 0.5);
        corps.lineTo(0.42, -0.2);
        corps.lineTo(-0.55, -0.2);
        corps.closeSubpath();
        p.drawPath(corps);
        p.drawEllipse(QPointF(0.88, -0.41), 0.12, 0.12);                      // volant
        p.drawLine(QPointF(-0.38, -0.2), QPointF(-0.38, 0.38));               // barre d'aiguille
        p.drawLine(QPointF(-0.38, 0.38), QPointF(-0.38, 0.5));                // aiguille
        p.drawLine(QPointF(-0.05, -0.62), QPointF(-0.05, -0.8));              // bobine
        p.drawLine(QPointF(-0.2, -0.8), QPointF(0.1, -0.8));
    } else if (type == "Nettoyage") {              // machine a laver (hublot + panneau de commande)
        p.drawRoundedRect(QRectF(-0.72, -0.95, 1.44, 1.9), 0.2, 0.2);
        p.drawLine(QPointF(-0.72, -0.5), QPointF(0.72, -0.5));                // panneau
        p.drawPoint(QPointF(-0.46, -0.72));                                    // voyants
        p.drawPoint(QPointF(-0.26, -0.72));
        p.drawEllipse(QPointF(0.4, -0.72), 0.1, 0.1);                         // bouton
        p.drawEllipse(QPointF(0.0, 0.2), 0.46, 0.46);                         // hublot
        p.drawArc(QRectF(-0.24, -0.04, 0.48, 0.48), 200 * 16, 220 * 16);     // linge / eau
    } else if (type == "Authentification") {       // ecusson + coche
        QPainterPath e;
        e.moveTo(0, -0.95);
        e.lineTo(0.8, -0.6);
        e.lineTo(0.75, 0.2);
        e.quadTo(0.58, 0.72, 0, 0.98);
        e.quadTo(-0.58, 0.72, -0.75, 0.2);
        e.lineTo(-0.8, -0.6);
        e.closeSubpath();
        p.drawPath(e);
        p.drawPolyline(QPolygonF({QPointF(-0.32, 0.0), QPointF(-0.06, 0.28), QPointF(0.4, -0.25)}));
    } else if (type == "Pressing") {               // fer a repasser : nez pointu a gauche + poignee
        QPainterPath f;
        f.moveTo(-0.95, 0.6);
        f.lineTo(0.9, 0.6);
        f.lineTo(0.9, 0.12);
        f.quadTo(0.9, -0.12, 0.62, -0.12);
        f.lineTo(0.0, -0.12);
        f.quadTo(-0.7, -0.08, -0.95, 0.6);
        f.closeSubpath();
        p.drawPath(f);
        p.drawRoundedRect(QRectF(-0.12, -0.58, 0.72, 0.46), 0.18, 0.18);       // poignee
        p.drawLine(QPointF(-0.45, 0.32), QPointF(0.6, 0.32));                  // semelle
    } else {                                       // controle qualite : loupe + coche
        p.drawEllipse(QPointF(-0.1, -0.1), 0.62, 0.62);
        p.drawLine(QPointF(0.36, 0.36), QPointF(0.92, 0.92));
        p.drawPolyline(QPolygonF({QPointF(-0.38, -0.1), QPointF(-0.14, 0.14), QPointF(0.2, -0.32)}));
    }
    p.restore();
}

QPixmap pixmapPictogrammePoste(const QString &type, const QColor &couleur, int taille)
{
    QPixmap pixmap = nouveauPixmap(taille);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(taille / 2.0, taille / 2.0);
    p.scale(taille * 0.42, taille * 0.42);
    dessinerPictogrammePoste(p, type, couleur);
    return pixmap;
}

void dessinerIcone(QPainter &p, Icone icone, const QRectF &zone, const QColor &couleur)
{
    p.save();
    p.translate(zone.topLeft());
    p.scale(zone.width(), zone.height());  // on dessine dans un carre unite [0,1] x [0,1]

    QPen stylo(couleur, 0.075, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(stylo);
    p.setBrush(Qt::NoBrush);

    switch (icone) {
    case Icone::Personne:
        p.drawEllipse(QPointF(0.5, 0.32), 0.17, 0.17);
        p.drawArc(QRectF(0.17, 0.56, 0.66, 0.8), 0, 180 * 16);
        break;
    case Icone::Groupe:
        p.drawEllipse(QPointF(0.36, 0.33), 0.13, 0.13);
        p.drawArc(QRectF(0.1, 0.55, 0.52, 0.7), 0, 180 * 16);
        p.drawEllipse(QPointF(0.68, 0.38), 0.11, 0.11);
        p.drawArc(QRectF(0.5, 0.58, 0.44, 0.6), 0, 180 * 16);
        break;
    case Icone::Mallette:
        p.drawRoundedRect(QRectF(0.1, 0.3, 0.8, 0.55), 0.08, 0.08);
        p.drawPolyline(QPolygonF({QPointF(0.35, 0.3), QPointF(0.35, 0.18), QPointF(0.65, 0.18), QPointF(0.65, 0.3)}));
        p.drawLine(QPointF(0.1, 0.56), QPointF(0.9, 0.56));
        break;
    case Icone::Enveloppe:
        p.drawRoundedRect(QRectF(0.08, 0.22, 0.84, 0.56), 0.06, 0.06);
        p.drawPolyline(QPolygonF({QPointF(0.08, 0.26), QPointF(0.5, 0.58), QPointF(0.92, 0.26)}));
        break;
    case Icone::Telephone:
        p.drawRoundedRect(QRectF(0.28, 0.08, 0.44, 0.84), 0.1, 0.1);
        p.drawLine(QPointF(0.43, 0.78), QPointF(0.57, 0.78));
        break;
    case Icone::Calendrier: {
        p.drawRoundedRect(QRectF(0.12, 0.2, 0.76, 0.68), 0.08, 0.08);
        p.drawLine(QPointF(0.12, 0.4), QPointF(0.88, 0.4));
        p.drawLine(QPointF(0.32, 0.1), QPointF(0.32, 0.27));
        p.drawLine(QPointF(0.68, 0.1), QPointF(0.68, 0.27));
        p.setPen(Qt::NoPen);
        p.setBrush(couleur);
        for (qreal y : {0.56, 0.72})
            for (qreal x : {0.32, 0.5, 0.68})
                p.drawEllipse(QPointF(x, y), 0.035, 0.035);
        break;
    }
    case Icone::Horloge:
        p.drawEllipse(QPointF(0.5, 0.5), 0.38, 0.38);
        p.drawLine(QPointF(0.5, 0.5), QPointF(0.5, 0.27));
        p.drawLine(QPointF(0.5, 0.5), QPointF(0.67, 0.6));
        break;
    case Icone::Loupe:
        p.drawEllipse(QPointF(0.44, 0.44), 0.30, 0.30);
        p.drawLine(QPointF(0.66, 0.66), QPointF(0.9, 0.9));
        break;
    case Icone::Repartition: {   // statistiques : ecran, barres et fleche montante
        p.drawRoundedRect(QRectF(0.08, 0.14, 0.84, 0.58), 0.04, 0.04);        // ecran
        p.drawLine(QPointF(0.5, 0.72), QPointF(0.5, 0.84));                   // pied
        p.drawLine(QPointF(0.33, 0.88), QPointF(0.67, 0.88));                 // socle
        for (qreal x : {0.24, 0.38, 0.52, 0.66, 0.80})                        // barres
            p.drawLine(QPointF(x, 0.64), QPointF(x, x < 0.5 ? 0.54 : 0.5));
        p.drawPolyline(QPolygonF({QPointF(0.18, 0.5), QPointF(0.38, 0.32),    // courbe qui monte
                                  QPointF(0.52, 0.44), QPointF(0.8, 0.2)}));
        p.drawPolyline(QPolygonF({QPointF(0.68, 0.2), QPointF(0.8, 0.2),      // pointe de la fleche
                                  QPointF(0.8, 0.32)}));
        break;
    }
    case Icone::Document: {
        QPainterPath page;
        page.moveTo(0.25, 0.1);
        page.lineTo(0.62, 0.1);
        page.lineTo(0.78, 0.26);
        page.lineTo(0.78, 0.9);
        page.lineTo(0.25, 0.9);
        page.closeSubpath();
        p.drawPath(page);
        p.drawPolyline(QPolygonF({QPointF(0.62, 0.1), QPointF(0.62, 0.26), QPointF(0.78, 0.26)}));
        p.drawLine(QPointF(0.36, 0.5), QPointF(0.67, 0.5));
        p.drawLine(QPointF(0.36, 0.64), QPointF(0.67, 0.64));
        p.drawLine(QPointF(0.36, 0.78), QPointF(0.55, 0.78));
        break;
    }
    }
    p.restore();
}

QPixmap pixmapIcone(Icone icone, const QColor &couleur, int taille)
{
    QPixmap pixmap = nouveauPixmap(taille);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    dessinerIcone(p, icone, QRectF(0, 0, taille, taille), couleur);
    return pixmap;
}
