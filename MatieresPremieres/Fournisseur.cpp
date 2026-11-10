// Fournisseur.cpp — requêtes SQL de l'entité Fournisseur (toutes préparées, valeurs liées par bindValue).
#include "Fournisseur.h"
#include "Database.h"
#include <QSqlDatabase>
#include <QSqlError>

Fournisseur::Fournisseur(const QString &nom, const QString &contact, const QString &telephone,
                         const QString &email, Database *db)
    : nom(nom), contact(contact), telephone(telephone), email(email), db_(db)
{
}

QSqlQuery Fournisseur::requete() const
{
    return QSqlQuery(db_->connection());   // toujours la connexion unique de l'application
}

// --- Exécute une requête préparée. Les valeurs ne sont JAMAIS concaténées dans le SQL
//     (protection contre l'injection SQL) : on met des « ? » puis on lie les valeurs.
bool Fournisseur::executer(const QString &sql, const QVariantList &valeurs) const
{
    QSqlQuery query = requete();
    query.prepare(sql);                              // 1. préparer
    for (const QVariant &v : valeurs)
        query.addBindValue(v);                       // 2. lier les valeurs
    if (!query.exec()) {                             // 3. exécuter
        db_->reportError(query);
        erreur_ = query.lastError().text();
        return false;
    }
    return true;
}

// --- Contrôle de saisie ---------------------------------------------------------------------
Stock::Errors Fournisseur::verifier() const
{
    Supplier s{nom.simplified(), contact.simplified(), telephone.trimmed(), email.trimmed()};
    return Stock::validateSupplier(s, noms(), ancienNom);   // nom obligatoire/unique/lettres, contact, téléphone, e-mail
}

QStringList Fournisseur::noms() const
{
    QStringList liste;
    if (!db_) return liste;
    QSqlQuery query = requete();
    if (query.exec("SELECT nom FROM suppliers ORDER BY LOWER(nom)"))
        while (query.next()) liste << query.value(0).toString();
    return liste;
}

int Fournisseur::nbMatieres(const QString &n) const
{
    if (!db_) return 0;
    QSqlQuery query = requete();
    query.prepare("SELECT COUNT(*) FROM materials WHERE fournisseur = ?");
    query.addBindValue(n);
    return (query.exec() && query.next()) ? query.value(0).toInt() : 0;
}

// --- Create : Ajouter -----------------------------------------------------------------------
bool Fournisseur::ajouter()
{
    erreur_.clear();
    if (!db_) { erreur_ = "Aucune connexion à la base de données."; return false; }
    if (!verifier().isEmpty()) { erreur_ = "Saisie invalide."; return false; }

    const bool ok = executer("INSERT INTO suppliers (nom, contact, telephone, email) VALUES (?, ?, ?, ?)",
                             {nom.simplified(), contact.simplified(), telephone.trimmed(), email.trimmed()});
    if (ok) {
        ancienNom = nom.simplified();
        db_->notifyChanged();                    // prévient les pages : la liste est rafraîchie
    }
    return ok;
}

// --- Read : Afficher ------------------------------------------------------------------------
// Retourne un modèle (QSqlQueryModel) à brancher sur un QTableView : ui->tableView->setModel(F.afficher()).
// Colonne 0 = nom (clé primaire) ; dernière colonne = nombre de matières liées.
QSqlQueryModel *Fournisseur::afficher(QObject *parent)
{
    auto *model = new QSqlQueryModel(parent);
    if (!db_) return model;
    model->setQuery("SELECT s.nom, s.contact, s.telephone, s.email, "
                    "(SELECT COUNT(*) FROM materials m WHERE m.fournisseur = s.nom) "
                    "FROM suppliers s ORDER BY LOWER(s.nom)",
                    db_->connection());
    if (model->lastError().isValid()) erreur_ = model->lastError().text();
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Contact"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Téléphone"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("E-mail"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Matières"));
    return model;
}

// --- Update : Modifier ----------------------------------------------------------------------
// Le fournisseur est identifié par son nom (clé primaire). S'il est renommé, les matières qui le citent
// sont mises à jour dans la même transaction.
bool Fournisseur::modifier()
{
    erreur_.clear();
    if (!db_) { erreur_ = "Aucune connexion à la base de données."; return false; }
    if (ancienNom.isEmpty()) { erreur_ = "Aucun fournisseur sélectionné."; return false; }
    if (!verifier().isEmpty()) { erreur_ = "Saisie invalide."; return false; }

    const QString nouveau = nom.simplified();
    QSqlDatabase conn = db_->connection();
    if (!conn.transaction()) { erreur_ = conn.lastError().text(); return false; }
    bool ok = executer("UPDATE suppliers SET nom = ?, contact = ?, telephone = ?, email = ? WHERE nom = ?",
                       {nouveau, contact.simplified(), telephone.trimmed(), email.trimmed(), ancienNom});
    if (ok && nouveau != ancienNom)
        ok = executer("UPDATE materials SET fournisseur = ? WHERE fournisseur = ?", {nouveau, ancienNom});
    if (!ok) { conn.rollback(); return false; }
    if (!conn.commit()) { erreur_ = conn.lastError().text(); conn.rollback(); return false; }
    ancienNom = nouveau;
    db_->notifyChanged();
    return true;
}

// --- Delete : Supprimer ---------------------------------------------------------------------
bool Fournisseur::supprimer(const QString &n)
{
    erreur_.clear();
    if (!db_) { erreur_ = "Aucune connexion à la base de données."; return false; }
    if (n.isEmpty()) { erreur_ = "Aucun fournisseur sélectionné."; return false; }

    const int utilise = nbMatieres(n);
    if (utilise > 0) {
        erreur_ = QString("Suppression impossible : %1 matière(s) utilisent ce fournisseur.").arg(utilise);
        return false;
    }
    const bool ok = executer("DELETE FROM suppliers WHERE nom = ?", {n});
    if (ok) db_->notifyChanged();
    return ok;
}
