// matiere.cpp — requêtes SQL de l'entité Matière première (préparées, valeurs liées par bindValue).
#include "matiere.h"
#include <QSqlError>
#include <QRegularExpression>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

// ---- Table Oracle du script v5 (une seule table ; catégorie et fournisseur = attributs texte) ----
#define T_MATIERE      "MATIERE_PREMIERE"
#define C_ID           "ID"
#define C_NOM          "NOM"
#define C_CATEGORIE    "CATEGORIE"
#define C_FOURNISSEUR  "FOURNISSEUR"
#define C_QUANTITE     "QUANTITE"
#define C_UNITE        "UNITE"
#define C_SEUIL        "SEUIL_ALERTE"
#define C_PRIX         "PRIX_UNITAIRE"
// L'ID vient d'une séquence Oracle dont le nom est lu dans USER_SEQUENCES (voir nomSequence()).
#ifndef SEQ_NEXT
#define SEQ_NEXT(seq)  ((seq) + ".NEXTVAL")
#endif

#define LISTE_COLONNES C_ID ", " C_NOM ", " C_CATEGORIE ", " C_FOURNISSEUR ", " C_QUANTITE ", " C_UNITE ", " C_SEUIL ", " C_PRIX

namespace Mat {

Matiere::Matiere(int id, const QString &nom, const QString &categorie, const QString &fournisseur,
                 double quantite, const QString &unite, double seuil, double prix)
    : id(id), nom(nom), categorie(categorie), fournisseur(fournisseur), unite(unite),
      quantite(quantite), seuil(seuil), prix(prix)
{
}

Matiere::Matiere(const Material &m)
    : Matiere(m.id, m.nom, m.categorie, m.fournisseur, m.quantite, m.unite, m.seuil, m.prix)
{
}

// Nom de la séquence qui alimente MATIERE_PREMIERE.ID : cherchée une seule fois dans le dictionnaire Oracle
// (nom contenant « MATIERE »). Vide = aucune trouvée (ID alimenté par la base : IDENTITY ou déclencheur).
static QString nomSequence()
{
    static bool cherche = false;
    static QString nom;
    if (cherche) return nom;
    cherche = true;
    QSqlQuery q;
    if (q.exec("SELECT SEQUENCE_NAME FROM USER_SEQUENCES WHERE SEQUENCE_NAME LIKE '%MATIERE%' ORDER BY SEQUENCE_NAME")
        && q.next()) {
        const QString n = q.value(0).toString();
        if (QRegularExpression("^[A-Za-z0-9_$#]+$").match(n).hasMatch()) nom = n;   // identifiant sûr
    }
    return nom;
}

// Traduit les erreurs Oracle les plus courantes en messages lisibles (le texte brut reste affiché après).
static QString messageErreur(const QSqlError &e)
{
    const QString t = e.text();
    if (t.contains("ORA-00001") || t.contains("UNIQUE", Qt::CaseInsensitive))
        return "Cette matière existe déjà (valeur en double refusée par la base).\n" + t;
    if (t.contains("ORA-02292") || t.contains("child record", Qt::CaseInsensitive) || t.contains("FOREIGN KEY", Qt::CaseInsensitive))
        return "Cette matière est utilisée ailleurs (table UTILISER) : suppression impossible.\n" + t;
    if (t.contains("ORA-02290") || t.contains("check constraint", Qt::CaseInsensitive))
        return "Une valeur ne respecte pas les contraintes de la table.\n" + t;
    if (t.contains("ORA-01400") || t.contains("ORA-02289"))
        return "L'ID n'a pas pu être généré : séquence de MATIERE_PREMIERE introuvable.\n" + t;
    if (t.contains("ORA-12899"))
        return "Un texte est trop long pour sa colonne.\n" + t;
    return t;
}

static Material lire(const QSqlRecord &r)
{
    Material m;
    m.id = r.value(0).toInt();
    m.nom = r.value(1).toString();
    m.categorie = r.value(2).toString();
    m.fournisseur = r.value(3).toString();
    m.quantite = r.value(4).toDouble();
    m.unite = r.value(5).toString();
    m.seuil = r.value(6).toDouble();
    m.prix = r.value(7).toDouble();
    return m;
}

// Contrôle de dernier recours dans la classe (la saisie détaillée est contrôlée avant, par Stock::validateMaterial).
bool Matiere::controler()
{
    erreur_.clear();
    if (nom.trimmed().isEmpty() || categorie.trimmed().isEmpty() || fournisseur.trimmed().isEmpty() || unite.trimmed().isEmpty()) {
        erreur_ = "Nom, catégorie, fournisseur et unité sont obligatoires.";
        return false;
    }
    // La base accepte seulement 'metre', 'bobine', 'piece' (CK_MP_UNITE, sans accents) : on tolère « mètre » / « pièce ».
    {
        QString u = unite.trimmed().toLower();
        u.replace(QString::fromUtf8("è"), "e").replace(QString::fromUtf8("é"), "e");
        if (u != "metre" && u != "bobine" && u != "piece") {
            erreur_ = "Unité invalide : choisissez metre, bobine ou piece.";
            return false;
        }
        unite = u;
    }
    if (quantite < 0 || seuil < 0 || prix < 0) {
        erreur_ = "Quantité, seuil et prix ne peuvent pas être négatifs.";
        return false;
    }
    return true;
}

bool Matiere::existe(const QString &n, int ignorerId) const
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM " T_MATIERE " WHERE LOWER(" C_NOM ") = LOWER(?) AND " C_ID " <> ?");
    query.addBindValue(n.trimmed());
    query.addBindValue(ignorerId);
    return query.exec() && query.next() && query.value(0).toInt() > 0;
}

// --- Create : Ajouter
bool Matiere::ajouter()
{
    if (!controler()) return false;
    if (existe(nom)) { erreur_ = "Cette matière existe déjà (le nom doit être unique)."; return false; }
    QSqlQuery query;
    const QString seq = nomSequence();
    if (!seq.isEmpty())    // ID = séquence Oracle
        query.prepare("INSERT INTO " T_MATIERE " (" LISTE_COLONNES ") VALUES (" + SEQ_NEXT(seq) + ", ?, ?, ?, ?, ?, ?, ?)");
    else                   // pas de séquence : l'ID est alimenté par la base (IDENTITY / déclencheur)
        query.prepare("INSERT INTO " T_MATIERE " (" C_NOM ", " C_CATEGORIE ", " C_FOURNISSEUR ", " C_QUANTITE ", "
                      C_UNITE ", " C_SEUIL ", " C_PRIX ") VALUES (?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(nom.trimmed());
    query.addBindValue(categorie.trimmed());
    query.addBindValue(fournisseur.trimmed());
    query.addBindValue(quantite);
    query.addBindValue(unite.trimmed());
    query.addBindValue(seuil);
    query.addBindValue(prix);
    if (!query.exec()) { erreur_ = messageErreur(query.lastError()); return false; }
    // retrouve l'ID attribué par la séquence (le nom est unique)
    QSqlQuery q2;
    q2.prepare("SELECT " C_ID " FROM " T_MATIERE " WHERE LOWER(" C_NOM ") = LOWER(?)");
    q2.addBindValue(nom.trimmed());
    if (q2.exec() && q2.next()) id = q2.value(0).toInt();
    return true;
}

// --- Read : Afficher
QSqlQueryModel *Matiere::afficher(QObject *parent)
{
    auto *model = new QSqlQueryModel(parent);
    model->setQuery("SELECT " LISTE_COLONNES " FROM " T_MATIERE " ORDER BY LOWER(" C_NOM ")");
    if (model->lastError().isValid()) erreur_ = messageErreur(model->lastError());
    const char *titres[] = {"ID", "Nom", "Catégorie", "Fournisseur", "Quantité", "Unité", "Seuil d'alerte", "Prix unitaire (DT)"};
    for (int i = 0; i < 8; ++i) model->setHeaderData(i, Qt::Horizontal, QObject::tr(titres[i]));
    return model;
}

// --- Update : Modifier
bool Matiere::modifier()
{
    if (!controler()) return false;
    if (id <= 0) { erreur_ = "Aucune matière sélectionnée."; return false; }
    if (existe(nom, id)) { erreur_ = "Une autre matière porte déjà ce nom (le nom doit être unique)."; return false; }
    QSqlQuery query;
    query.prepare("UPDATE " T_MATIERE " SET " C_NOM " = ?, " C_CATEGORIE " = ?, " C_FOURNISSEUR " = ?, "
                  C_QUANTITE " = ?, " C_UNITE " = ?, " C_SEUIL " = ?, " C_PRIX " = ? WHERE " C_ID " = ?");
    query.addBindValue(nom.trimmed());
    query.addBindValue(categorie.trimmed());
    query.addBindValue(fournisseur.trimmed());
    query.addBindValue(quantite);
    query.addBindValue(unite.trimmed());
    query.addBindValue(seuil);
    query.addBindValue(prix);
    query.addBindValue(id);
    if (!query.exec()) { erreur_ = messageErreur(query.lastError()); return false; }
    if (query.numRowsAffected() == 0) { erreur_ = "Cette matière n'existe plus."; return false; }
    return true;
}

// --- Delete : Supprimer
bool Matiere::supprimer(int idASupprimer)
{
    erreur_.clear();
    if (idASupprimer <= 0) { erreur_ = "Aucune matière sélectionnée."; return false; }
    QSqlQuery query;
    query.prepare("DELETE FROM " T_MATIERE " WHERE " C_ID " = ?");
    query.addBindValue(idASupprimer);
    if (!query.exec()) { erreur_ = messageErreur(query.lastError()); return false; }
    if (query.numRowsAffected() == 0) { erreur_ = "Cette matière n'existe plus."; return false; }
    return true;
}

// Lecture complète : passe par afficher() (le modèle est lu en entier, puis converti en structures).
QVector<Material> Matiere::tous(QString *erreur)
{
    QVector<Material> v;
    Matiere M;
    QSqlQueryModel *model = M.afficher();
    while (model->canFetchMore()) model->fetchMore();
    if (!M.erreur_.isEmpty() && erreur) *erreur = M.erreur_;
    for (int r = 0; r < model->rowCount(); ++r) v.push_back(lire(model->record(r)));
    delete model;
    return v;
}

// Mouvement de stock : seule la quantité change.
bool Matiere::changerQuantite(int id, double quantite, QString *erreur)
{
    QSqlQuery query;
    query.prepare("UPDATE " T_MATIERE " SET " C_QUANTITE " = ? WHERE " C_ID " = ?");
    query.addBindValue(quantite);
    query.addBindValue(id);
    if (!query.exec()) { if (erreur) *erreur = messageErreur(query.lastError()); return false; }
    if (query.numRowsAffected() == 0) { if (erreur) *erreur = "Cette matière n'existe plus."; return false; }
    return true;
}

// Vérifie que la table existe avec les colonnes attendues (message précis sinon).
bool Matiere::tableAccessible(QString *erreur)
{
    QSqlQuery query;
    if (!query.exec("SELECT " LISTE_COLONNES " FROM " T_MATIERE " WHERE 1 = 0")) {
        if (erreur) *erreur = query.lastError().text();
        return false;
    }
    return true;
}

}  // namespace Mat
