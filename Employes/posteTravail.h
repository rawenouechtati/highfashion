#ifndef POSTETRAVAIL_H
#define POSTETRAVAIL_H

#include <QString>
#include <QStringList>

// Postes de travail de l'atelier (emplacements physiques ou un employe travaille).
// Il n'y a plus de table POSTE_TRAVAIL : le poste d'un employe est simplement le texte de
// la colonne EMPLOYE.POSTE_TRAVAIL (NULL = aucun poste), limite par une contrainte CHECK a
// la liste ci-dessous et UNIQUE (un poste n'a qu'un occupant a la fois). Cette classe n'a
// donc ni acces a la base ni etat : elle porte seulement la liste fixe des 6 postes, comme
// constante cote C++, et quelques conversions d'affichage.
class PosteTravail
{
public:
    // Les 6 valeurs autorisees par la contrainte CHECK, dans l'ordre d'affichage.
    static QStringList tous();

    static bool estValide(const QString &poste);

    // Libelle complet pour l'affichage : "Couture 1" -> "Poste de couture 1".
    static QString libelle(const QString &poste);

    // Nom court pour les petites cartes : "Couture 1" -> "Couture 1", "Controle qualite" -> "Contrôle qualité".
    static QString libelleCourt(const QString &poste);

    // Famille du poste, pour choisir son pictogramme : "Couture 1" -> "Couture".
    static QString type(const QString &poste);
};

#endif // POSTETRAVAIL_H
