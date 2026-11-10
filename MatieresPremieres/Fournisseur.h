// Fournisseur.h — classe MODÈLE de l'entité « Fournisseur » (atelier CRUD, architecture Modèle-Vue).
//
// Règle de l'atelier : AUCUNE requête SQL derrière un bouton. Toutes les requêtes du fournisseur
// sont ici, dans cette classe C++ ; l'interface (Pages.cpp / SuppliersPage) ne fait qu'appeler
// ajouter(), afficher(), modifier() et supprimer().
//
// Table Oracle correspondante : SUPPLIERS (NOM = clé primaire, CONTACT, TELEPHONE, EMAIL)
// La connexion unique à la base (patron Singleton) est celle de la classe Database : on la réutilise.
#ifndef FOURNISSEUR_H
#define FOURNISSEUR_H

#include "Stock.h"
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QString>
#include <QStringList>
#include <QVariantList>

class Database;

class Fournisseur
{
    QString nom, contact, telephone, email;   // nom = clé primaire
    QString ancienNom;                // nom actuel en base lors d'une modification (vide = nouveau fournisseur)
    Database *db_ = nullptr;          // connexion unique réutilisée (non possédée)
    mutable QString erreur_;          // dernier message d'erreur compréhensible

public:
    // Constructeurs
    explicit Fournisseur(Database *db = nullptr) : db_(db) {}
    Fournisseur(const QString &nom, const QString &contact, const QString &telephone,
                const QString &email, Database *db = nullptr);

    // Getters
    QString getNom() const { return nom; }
    QString getContact() const { return contact; }
    QString getTelephone() const { return telephone; }
    QString getEmail() const { return email; }

    // Setters
    void setNom(const QString &n) { nom = n; }
    void setAncienNom(const QString &n) { ancienNom = n; }   // nom du fournisseur à modifier (vide = nouveau)
    void setContact(const QString &c) { contact = c; }
    void setTelephone(const QString &t) { telephone = t; }
    void setEmail(const QString &e) { email = e; }

    // Contrôle de saisie (dans le code C++) : retourne les erreurs par champ, vide = valide.
    // En modification (ancienNom renseigné), son propre nom est ignoré pour le test de doublon.
    Stock::Errors verifier() const;

    // Fonctionnalités de base relatives à l'entité Fournisseur (CRUD)
    bool ajouter();                                  // Create
    QSqlQueryModel *afficher(QObject *parent = nullptr);   // Read  : modèle pour un QTableView
    bool modifier();                                 // Update : modifie le fournisseur nommé ancienNom
    bool supprimer(const QString &nom);              // Delete : refusé si des matières l'utilisent

    // Outils
    int nbMatieres(const QString &nom) const;        // nombre de matières premières liées
    QStringList noms() const;                        // tous les noms existants
    QString erreur() const { return erreur_; }       // message de la dernière opération échouée

private:
    bool executer(const QString &sql, const QVariantList &valeurs) const;   // requête préparée + bindValue
    QSqlQuery requete() const;
};

#endif // FOURNISSEUR_H
