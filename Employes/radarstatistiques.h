#ifndef RADARSTATISTIQUES_H
#define RADARSTATISTIQUES_H

#include <QMap>
#include <QString>
#include <QStringList>

class QChart;

// Construit un diagramme araignee (radar) : un axe par entree de "axes", dans l'ordre
// donne, dont la longueur est la valeur correspondante de "valeurs" (0 si absente).
// Fonction d'affichage pure : elle ne lit jamais la base, les donnees lui sont fournies.
// L'appelant recupere la propriete du QChart (ex. en le donnant a un QChartView).
QChart *creerDiagrammeAraignee(const QString &titre, const QStringList &axes,
                               const QMap<QString, int> &valeurs);

#endif // RADARSTATISTIQUES_H
