// Matiere.h — classe MODÈLE de l'entité « Matière première » (entité du module, atelier CRUD).
//
// Comme pour Fournisseur : toutes les requêtes SQL de la matière première sont dans cette classe
// C++ (requêtes préparées + bindValue). La page (MaterialsPage) ne fait qu'appeler ajouter(),
// afficher(), modifier() et supprimer().
// Table Oracle : MATERIALS (NOM = clé primaire, CATEGORIE et FOURNISSEUR = clés étrangères).
#ifndef MATIERE_H
#define MATIERE_H

#include "Models.h"
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QString>
#include <QVariantList>

class Database;

class Matiere
{
    QString nom, categorie, fournisseur, unite;   // nom = clé primaire
    double quantite = 0, seuil = 0, prix = 0;
    QString ancienNom;                // nom actuel en base lors d'une modification (vide = identique à nom)
    Database *db_ = nullptr;          // connexion unique réutilisée (non possédée)
    mutable QString erreur_;

public:
    // Constructeurs
    explicit Matiere(Database *db = nullptr) : db_(db) {}
    Matiere(const QString &nom, const QString &categorie, const QString &fournisseur,
            double quantite, const QString &unite, double seuil, double prix, Database *db = nullptr);
    Matiere(const Material &m, Database *db);      // à partir de la structure utilisée par l'interface

    // Getters
    QString getNom() const { return nom; }
    QString getCategorie() const { return categorie; }
    QString getFournisseur() const { return fournisseur; }
    QString getUnite() const { return unite; }
    double getQuantite() const { return quantite; }
    double getSeuil() const { return seuil; }
    double getPrix() const { return prix; }

    // Setters
    void setNom(const QString &v) { nom = v; }
    void setAncienNom(const QString &v) { ancienNom = v; }   // à appeler avant modifier() si le nom peut changer
    void setCategorie(const QString &v) { categorie = v; }
    void setFournisseur(const QString &v) { fournisseur = v; }
    void setUnite(const QString &v) { unite = v; }
    void setQuantite(double v) { quantite = v; }
    void setSeuil(double v) { seuil = v; }
    void setPrix(double v) { prix = v; }

    // Fonctionnalités de base relatives à l'entité Matière première (CRUD)
    bool ajouter();                                        // Create
    QSqlQueryModel *afficher(QObject *parent = nullptr);   // Read : modèle pour un QTableView
    bool modifier();                                       // Update : la matière dont le nom actuel est ancienNom
    bool supprimer(const QString &nom);                    // Delete : l'historique des mouvements est conservé

    // Outils
    bool existe(const QString &nom) const;
    QString erreur() const { return erreur_; }

private:
    bool controler();                                      // contrôle minimal avant écriture
    bool executer(const QString &sql, const QVariantList &valeurs) const;
};

#endif // MATIERE_H
