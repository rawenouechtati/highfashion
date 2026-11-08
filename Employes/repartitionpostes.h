#ifndef REPARTITIONPOSTES_H
#define REPARTITIONPOSTES_H

#include <QList>
#include <QPair>
#include <QString>
#include <QWidget>

// "Repartition par poste" : une barre horizontale partagee en segments colores (un par poste,
// proportionnel au nombre d'employes) avec sa legende en dessous. Remplace l'ancien
// histogramme, plus lourd pour une information aussi simple. Le widget ne lit jamais la
// base : on lui donne les donnees avec setDonnees().
class RepartitionPostes : public QWidget
{
    Q_OBJECT

public:
    explicit RepartitionPostes(QWidget *parent = nullptr);

    // parts : (libelle, nombre d'employes), dans l'ordre d'affichage.
    void setDonnees(const QList<QPair<QString, int>> &parts);

    QSize sizeHint() const override { return QSize(520, 92); }
    QSize minimumSizeHint() const override { return QSize(280, 90); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<QPair<QString, int>> m_parts;
};

#endif // REPARTITIONPOSTES_H
