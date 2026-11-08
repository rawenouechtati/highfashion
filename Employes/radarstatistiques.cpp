#include "radarstatistiques.h"

#include <QAreaSeries>
#include <QCategoryAxis>
#include <QColor>
#include <QFont>
#include <QLineSeries>
#include <QPen>
#include <QPolarChart>
#include <QValueAxis>
#include <algorithm>

QChart *creerDiagrammeAraignee(const QString &titre, const QStringList &axes,
                               const QMap<QString, int> &valeurs)
{
    const int n = axes.size();
    int maximum = 1;
    for (const QString &axe : axes)
        maximum = std::max(maximum, valeurs.value(axe, 0));

    // Un point par axe, espaces regulierement sur 360 degres, puis retour au premier
    // point pour fermer le polygone.
    auto *contour = new QLineSeries();
    const qreal pas = n > 0 ? 360.0 / n : 360.0;
    for (int i = 0; i < n; ++i)
        contour->append(i * pas, valeurs.value(axes.at(i), 0));
    if (n > 0)
        contour->append(360.0, valeurs.value(axes.first(), 0));

    const QColor bordeaux("#8B1E2F");
    QPen stylo(bordeaux);
    stylo.setWidthF(2.2);
    contour->setPen(stylo);

    auto *surface = new QAreaSeries(contour);
    QColor remplissage = bordeaux;
    remplissage.setAlpha(80);
    surface->setBrush(remplissage);
    surface->setPen(stylo);

    auto *graphique = new QPolarChart();
    graphique->addSeries(surface);
    graphique->setTitle(titre);
    graphique->setTitleFont(QFont("Playfair Display", 11, QFont::Bold));
    graphique->setTitleBrush(QColor("#1F3A4D"));
    graphique->legend()->hide();
    graphique->setBackgroundBrush(Qt::NoBrush);

    // Axe angulaire : un libelle par axe, pose sur sa valeur (le dernier categorie se
    // termine a 360 degres, qui est aussi la position 0 du premier axe).
    auto *axeAngulaire = new QCategoryAxis();
    axeAngulaire->setMin(0);
    axeAngulaire->setMax(360);
    axeAngulaire->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    axeAngulaire->setLabelsColor(QColor("#1F3A4D"));
    axeAngulaire->setGridLineColor(QColor("#B39B6A"));
    for (int i = 1; i < n; ++i)
        axeAngulaire->append(axes.at(i), i * pas);
    if (n > 0)
        axeAngulaire->append(axes.first(), 360.0);
    graphique->addAxis(axeAngulaire, QPolarChart::PolarOrientationAngular);

    // Axe radial : graduations entieres (ce sont des nombres de services).
    auto *axeRadial = new QValueAxis();
    axeRadial->setRange(0, maximum);
    axeRadial->setTickCount(std::min(maximum, 5) + 1);
    axeRadial->setLabelFormat("%d");
    axeRadial->setLabelsFont(QFont("Lato", 7));
    axeRadial->setLabelsColor(QColor("#8A8375"));
    axeRadial->setGridLineColor(QColor("#E6DCC8"));
    graphique->addAxis(axeRadial, QPolarChart::PolarOrientationRadial);

    surface->attachAxis(axeAngulaire);
    surface->attachAxis(axeRadial);
    contour->attachAxis(axeAngulaire);
    contour->attachAxis(axeRadial);

    return graphique;
}
