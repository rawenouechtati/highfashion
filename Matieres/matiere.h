// matiere.h — classe ENTITÉ « Matière première » (table Oracle MATIERE_PREMIERE du script v5).
//
// Atelier CRUD : TOUTES les requêtes SQL de la matière première sont écrites dans cette classe
// (requêtes préparées + bindValue, connexion unique ouverte par main.cpp). Les pages et la
// fenêtre principale ne font qu'appeler ajouter(), afficher(), modifier() et supprimer().
#ifndef MATIERE_H
#define MATIERE_H

#include "matmodels.h"
#include <QSqlQueryModel>
#include <QString>
#include <QStringList>
#include <QVector>

namespace Mat {

class Matiere
{
    int id = 0;                                   // ID (clé primaire Oracle, séquence)
    QString nom, categorie, fournisseur, unite;
    double quantite = 0, seuil = 0, prix = 0;
    mutable QString erreur_;

public:
    // Constructeurs
    Matiere() = default;
    Matiere(int id, const QString &nom, const QString &categorie, const QString &fournisseur,
            double quantite, const QString &unite, double seuil, double prix);
    explicit Matiere(const Material &m);          // à partir de la structure utilisée par l'interface

    // Getters
    int getId() const { return id; }
    QString getNom() const { return nom; }
    QString getCategorie() const { return categorie; }
    QString getFournisseur() const { return fournisseur; }
    QString getUnite() const { return unite; }
    double getQuantite() const { return quantite; }
    double getSeuil() const { return seuil; }
    double getPrix() const { return prix; }

    // Setters
    void setId(int v) { id = v; }
    void setNom(const QString &v) { nom = v; }
    void setCategorie(const QString &v) { categorie = v; }
    void setFournisseur(const QString &v) { fournisseur = v; }
    void setUnite(const QString &v) { unite = v; }
    void setQuantite(double v) { quantite = v; }
    void setSeuil(double v) { seuil = v; }
    void setPrix(double v) { prix = v; }

    // Fonctionnalités de base relatives à l'entité (CRUD)
    bool ajouter();                                        // Create : l'ID vient de la séquence Oracle
    QSqlQueryModel *afficher(QObject *parent = nullptr);   // Read : modèle (QTableView)
    bool modifier();                                       // Update : la matière d'ID « id »
    bool supprimer(int id);                                // Delete

    // Outils (toujours dans l'entité : aucune requête dans les pages)
    static QVector<Material> tous(QString *erreur = nullptr);   // lit toute la table (via afficher())
    static bool changerQuantite(int id, double quantite, QString *erreur = nullptr);   // mouvement de stock
    static bool tableAccessible(QString *erreur = nullptr);     // la table et ses colonnes existent-elles ?
    bool existe(const QString &nom, int ignorerId = 0) const;   // nom déjà pris par une autre matière ?
    QString erreur() const { return erreur_; }

private:
    bool controler();                                      // contrôle minimal avant écriture
};

}  // namespace Mat

#endif  // MATIERE_H
