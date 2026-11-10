// Matiere.cpp — requêtes SQL de l'entité Matière première (préparées, valeurs liées par bindValue).
#include "Matiere.h"
#include "Database.h"
#include <QSqlDatabase>
#include <QSqlError>

Matiere::Matiere(const QString &nom, const QString &categorie, const QString &fournisseur,
                 double quantite, const QString &unite, double seuil, double prix, Database *db)
    : nom(nom), categorie(categorie), fournisseur(fournisseur), unite(unite),
      quantite(quantite), seuil(seuil), prix(prix), db_(db)
{
}

Matiere::Matiere(const Material &m, Database *db)
    : Matiere(m.nom, m.categorie, m.fournisseur, m.quantite, m.unite, m.seuil, m.prix, db)
{
}

bool Matiere::executer(const QString &sql, const QVariantList &valeurs) const
{
    QSqlQuery query(db_->connection());
    query.prepare(sql);                                  // 1. préparer
    for (const QVariant &v : valeurs) query.addBindValue(v);   // 2. lier les valeurs
    if (!query.exec()) {                                 // 3. exécuter
        db_->reportError(query);
        erreur_ = query.lastError().text();
        return false;
    }
    return true;
}

// Contrôle de dernier recours dans la classe (la saisie détaillée est contrôlée avant, par Stock::validateMaterial).
bool Matiere::controler()
{
    erreur_.clear();
    if (!db_) { erreur_ = "Aucune connexion à la base de données."; return false; }
    if (nom.trimmed().isEmpty() || categorie.isEmpty() || fournisseur.isEmpty() || unite.isEmpty()) {
        erreur_ = "Nom, catégorie, fournisseur et unité sont obligatoires.";
        return false;
    }
    if (quantite < 0 || seuil < 0 || prix < 0) { erreur_ = "Quantité, seuil et prix ne peuvent pas être négatifs."; return false; }
    return true;
}

bool Matiere::existe(const QString &n) const
{
    if (!db_) return false;
    QSqlQuery query(db_->connection());
    query.prepare("SELECT COUNT(*) FROM materials WHERE LOWER(nom) = LOWER(?)");
    query.addBindValue(n);
    return query.exec() && query.next() && query.value(0).toInt() > 0;
}

// --- Create : Ajouter
bool Matiere::ajouter()
{
    if (!controler()) return false;
    if (existe(nom)) { erreur_ = "Cette matière existe déjà (le nom est la clé primaire)."; return false; }
    const bool ok = executer("INSERT INTO materials (nom, categorie, fournisseur, quantite, unite, seuil, prix) "
                             "VALUES (?, ?, ?, ?, ?, ?, ?)",   // categorie et fournisseur : clés étrangères (noms)
                             {nom, categorie, fournisseur, quantite, unite, seuil, prix});
    if (ok) db_->notifyChanged();          // les pages rafraîchissent leur affichage
    return ok;
}

// --- Read : Afficher
QSqlQueryModel *Matiere::afficher(QObject *parent)
{
    auto *model = new QSqlQueryModel(parent);
    if (!db_) return model;
    model->setQuery("SELECT nom, categorie, fournisseur, quantite, unite, seuil, prix "
                    "FROM materials ORDER BY LOWER(nom)",
                    db_->connection());
    if (model->lastError().isValid()) erreur_ = model->lastError().text();
    const char *titres[] = {"Nom", "Catégorie", "Fournisseur", "Quantité", "Unité", "Seuil d'alerte", "Prix unitaire (DT)"};
    for (int i = 0; i < 7; ++i) model->setHeaderData(i, Qt::Horizontal, QObject::tr(titres[i]));
    return model;
}

// --- Update : Modifier (la clé primaire est le nom ; si elle change, l'historique et l'alerte suivent)
bool Matiere::modifier()
{
    if (!controler()) return false;
    const QString ancien = ancienNom.isEmpty() ? nom : ancienNom;
    const bool renomme = ancien != nom;
    if (!existe(ancien)) { erreur_ = "Cette matière n'existe plus."; return false; }
    if (renomme && ancien.toLower() != nom.toLower() && existe(nom)) {
        erreur_ = "Une autre matière porte déjà ce nom (le nom est la clé primaire).";
        return false;
    }

    QSqlDatabase conn = db_->connection();
    if (!conn.transaction()) { erreur_ = conn.lastError().text(); return false; }
    bool ok = executer("UPDATE materials SET nom = ?, categorie = ?, fournisseur = ?, quantite = ?, "
                       "unite = ?, seuil = ?, prix = ? WHERE nom = ?",
                       {nom, categorie, fournisseur, quantite, unite, seuil, prix, ancien});
    if (ok && renomme) {   // les tables qui citent la matière par son nom suivent le changement
        ok = executer("UPDATE movements SET material_id = ?, nom = ? WHERE material_id = ?", {nom, nom, ancien}) &&
             executer("UPDATE alerts SET material_id = ?, nom = ? WHERE material_id = ?", {nom, nom, ancien});
    }
    if (!ok) { conn.rollback(); return false; }
    if (!conn.commit()) { erreur_ = conn.lastError().text(); conn.rollback(); return false; }
    ancienNom = nom;
    db_->notifyChanged();
    return true;
}

// --- Delete : Supprimer
bool Matiere::supprimer(const QString &n)
{
    erreur_.clear();
    if (!db_) { erreur_ = "Aucune connexion à la base de données."; return false; }
    if (n.isEmpty()) { erreur_ = "Aucune matière sélectionnée."; return false; }
    // L'alerte liée disparaît aussi (ON DELETE CASCADE sur Oracle ; DELETE explicite pour SQLite).
    const bool ok = executer("DELETE FROM alerts WHERE material_id = ?", {n}) &&
                    executer("DELETE FROM materials WHERE nom = ?", {n});
    if (ok) db_->notifyChanged();
    return ok;
}
