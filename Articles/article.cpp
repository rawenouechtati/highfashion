#include "article.h"
#include <QDate>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <cmath>

// ---- Oracle : séquence, dernier ID et conversion de date (redéfinis seulement par la base de test SQLite) ----
#ifndef ART_NEXT_ID
#define ART_NEXT_ID  "ARTICLE_SEQ.NEXTVAL"
#define ART_LAST_ID  "SELECT ARTICLE_SEQ.CURRVAL FROM DUAL"
#define ART_DATE(p)  "TO_DATE(" p ", 'YYYY-MM-DD')"
#endif

// ---- table ARTICLE (v5) : ID, NOM, MAISON, DECENNIE, ETAT, PRIX, CATEGORIE, ORIGINE, STATUT, ID_SERVICE, DATE_ARTICLE ----
#define LISTE_COLONNES "ID, NOM, MAISON, DECENNIE, ETAT, PRIX, CATEGORIE, ORIGINE, STATUT, ID_SERVICE, DATE_ARTICLE"

Article::Article(int id, const QString &nom, const QString &maison, int decennie, const QString &etat, double prix,
                 const QString &categorie, const QString &origine, const QString &statut, int idService,
                 const QDate &date)
    : id(id), nom(nom), maison(maison), decennie(decennie), etat(etat), prix(prix), categorie(categorie),
      origine(origine), statut(statut), idService(idService), date(date)
{}

// ----------------------------------------------------------------------------------------------------------
// CRUD
// ----------------------------------------------------------------------------------------------------------

bool Article::ajouter()
{
    erreur_.clear();
    const Saisie s{nom, maison, QString::number(decennie), etat, QString::number(prix, 'f', 3), categorie, origine, statut,
                   idService, date};
    const QMap<QString, QString> e = verifier(s);
    if (!e.isEmpty()) { erreur_ = e.first(); return false; }
    if (existe(nom, maison)) { erreur_ = "Cet article existe déjà (même nom et même maison de couture)."; return false; }

    QSqlQuery query;
    query.prepare("INSERT INTO ARTICLE (" LISTE_COLONNES ") VALUES (" ART_NEXT_ID ", :nom, :maison, :decennie, :etat, "
                  ":prix, :categorie, :origine, :statut, :id_service, " ART_DATE(":date") ")");
    query.bindValue(":nom", nom.simplified());
    query.bindValue(":maison", maison.simplified());
    query.bindValue(":decennie", decennie);
    query.bindValue(":etat", etat);
    query.bindValue(":prix", prix);
    query.bindValue(":categorie", categorie.simplified());
    query.bindValue(":origine", origine.simplified());
    query.bindValue(":statut", statut);
    query.bindValue(":id_service", idService);
    query.bindValue(":date", date.toString("yyyy-MM-dd"));
    if (!query.exec()) { erreur_ = messageErreur(query.lastError().text()); return false; }

    QSqlQuery q2;                      // ID que la séquence vient de donner (même session Oracle)
    if (q2.exec(ART_LAST_ID) && q2.next()) id = q2.value(0).toInt();
    return true;
}

QSqlQueryModel *Article::afficher()
{
    auto *model = new QSqlQueryModel;
    model->setQuery("SELECT " LISTE_COLONNES " FROM ARTICLE ORDER BY ID");
    const char *titres[] = {"ID", "Nom", "Maison", "Décennie", "État", "Prix (DT)", "Catégorie", "Origine", "Statut", "Service", "Date"};
    for (int i = 0; i < 11; ++i) model->setHeaderData(i, Qt::Horizontal, QString::fromUtf8(titres[i]));
    return model;
}

bool Article::modifier()
{
    erreur_.clear();
    if (id <= 0) { erreur_ = "Aucun article sélectionné."; return false; }
    const Saisie s{nom, maison, QString::number(decennie), etat, QString::number(prix, 'f', 3), categorie, origine, statut,
                   idService, date};
    const QMap<QString, QString> e = verifier(s);
    if (!e.isEmpty()) { erreur_ = e.first(); return false; }
    if (existe(nom, maison, id)) { erreur_ = "Un autre article porte déjà ce nom pour cette maison de couture."; return false; }

    QSqlQuery query;
    query.prepare("UPDATE ARTICLE SET NOM = :nom, MAISON = :maison, DECENNIE = :decennie, ETAT = :etat, PRIX = :prix, "
                  "CATEGORIE = :categorie, ORIGINE = :origine, STATUT = :statut, ID_SERVICE = :id_service, "
                  "DATE_ARTICLE = " ART_DATE(":date") " WHERE ID = :id");
    query.bindValue(":nom", nom.simplified());
    query.bindValue(":maison", maison.simplified());
    query.bindValue(":decennie", decennie);
    query.bindValue(":etat", etat);
    query.bindValue(":prix", prix);
    query.bindValue(":categorie", categorie.simplified());
    query.bindValue(":origine", origine.simplified());
    query.bindValue(":statut", statut);
    query.bindValue(":id_service", idService);
    query.bindValue(":date", date.toString("yyyy-MM-dd"));
    query.bindValue(":id", id);
    if (!query.exec()) { erreur_ = messageErreur(query.lastError().text()); return false; }
    if (query.numRowsAffected() == 0) { erreur_ = "Cet article n'existe plus dans la base (supprimé depuis un autre poste ?)."; return false; }
    return true;
}

bool Article::supprimer(int idArticle)
{
    erreur_.clear();
    QSqlQuery query;
    query.prepare("DELETE FROM ARTICLE WHERE ID = :id");
    query.bindValue(":id", idArticle);
    if (!query.exec()) { erreur_ = messageErreur(query.lastError().text()); return false; }
    if (query.numRowsAffected() == 0) { erreur_ = "Cet article n'existe plus dans la base."; return false; }
    return true;
}

// ----------------------------------------------------------------------------------------------------------
// Lectures
// ----------------------------------------------------------------------------------------------------------

QVector<Article> Article::tous(QString *erreur)
{
    QVector<Article> liste;
    Article modele;
    QSqlQueryModel *m = modele.afficher();
    if (m->lastError().isValid()) {
        if (erreur) *erreur = messageErreur(m->lastError().text());
        delete m;
        return liste;
    }
    while (m->canFetchMore()) m->fetchMore();
    for (int r = 0; r < m->rowCount(); ++r) {
        auto v = [&](int c) { return m->data(m->index(r, c)); };
        liste << Article(v(0).toInt(), v(1).toString(), v(2).toString(), v(3).toInt(), v(4).toString(),
                         v(5).toDouble(), v(6).toString(), v(7).toString(), v(8).toString(), v(9).toInt(),
                         v(10).toDate());
    }
    delete m;
    return liste;
}

bool Article::tableAccessible(QString *erreur)
{
    QSqlQuery q;
    if (q.exec("SELECT " LISTE_COLONNES " FROM ARTICLE WHERE 1 = 0")) return true;
    if (erreur) *erreur = q.lastError().text();
    return false;
}

bool Article::existe(const QString &nom, const QString &maison, int ignorerId)
{
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM ARTICLE WHERE UPPER(NOM) = UPPER(:nom) AND UPPER(MAISON) = UPPER(:maison) AND ID <> :id");
    q.bindValue(":nom", nom.simplified());
    q.bindValue(":maison", maison.simplified());
    q.bindValue(":id", ignorerId);
    return q.exec() && q.next() && q.value(0).toInt() > 0;
}

QList<int> Article::services(QString *erreur)
{
    QList<int> ids;
    QSqlQuery q;
    if (!q.exec("SELECT ID FROM SERVICE ORDER BY ID")) {
        if (erreur) *erreur = q.lastError().text();
        return ids;
    }
    while (q.next()) ids << q.value(0).toInt();
    return ids;
}

QString Article::messageErreur(const QString &t)
{
    if (t.contains("ORA-00001") || t.contains("UNIQUE constraint"))
        return "Cet article existe déjà (même nom et même maison de couture).";
    if (t.contains("ORA-02291") || t.contains("FOREIGN KEY"))
        return "Le service choisi n'existe pas dans la table SERVICE.";
    if (t.contains("ORA-02292") || t.contains("child record"))
        return "Suppression impossible : cet article est utilisé ailleurs dans la base.";
    if (t.contains("ORA-02290") || t.contains("check constraint") || t.contains("CHECK constraint"))
        return "Une valeur n'est pas acceptée par la base (contrainte CHECK) : " + t.section("(", 1, 1).section(")", 0, 0);
    if (t.contains("ORA-01400") || t.contains("NOT NULL"))
        return "Un champ obligatoire est vide.";
    if (t.contains("ORA-12899") || t.contains("ORA-01438"))
        return "Une valeur est trop longue ou trop grande pour la colonne.";
    if (t.contains("ORA-02289"))
        return "La séquence ARTICLE_SEQ n'existe pas dans la base.";
    return t.simplified();
}

// ----------------------------------------------------------------------------------------------------------
// Listes proposées
// ----------------------------------------------------------------------------------------------------------

// Valeurs acceptées par les contraintes CHECK de la table (ex. ETAT IN ('Neuf','Bon',...)) : elles sont LUES dans
// USER_CONSTRAINTS pour que les listes de l'interface ne proposent jamais une valeur refusée par Oracle.
// Si la lecture est impossible, on retombe sur les valeurs par défaut ci-dessous.
static QMap<QString, QStringList> &contraintesCheck(bool relire = false)
{
    static QMap<QString, QStringList> m;
    static bool lu = false;
    if (relire) lu = false;
    if (lu) return m;
    lu = true;
    m.clear();
    QSqlQuery q;
    if (!q.exec("SELECT SEARCH_CONDITION FROM USER_CONSTRAINTS WHERE TABLE_NAME = 'ARTICLE' AND CONSTRAINT_TYPE = 'C'"))
        return m;
    while (q.next())
        Article::analyserCondition(q.value(0).toString(), m);
    return m;
}

void Article::analyserCondition(const QString &condition, QMap<QString, QStringList> &out)
{
    // « COLONNE IN ('a', 'b', 'c') »  (éventuellement UPPER(COLONNE) IN (...))
    static const QRegularExpression re(R"RE((?:UPPER\(|LOWER\()?\s*"?(\w+)"?\s*\)?\s+IN\s*\(([^)]*)\))RE",
                                       QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = re.globalMatch(condition);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        QStringList valeurs;
        static const QRegularExpression val("'((?:[^']|'')*)'");
        QRegularExpressionMatchIterator vi = val.globalMatch(m.captured(2));
        while (vi.hasNext()) valeurs << vi.next().captured(1).replace("''", "'");
        if (!valeurs.isEmpty()) out[m.captured(1).toUpper()] = valeurs;
    }
}

void Article::relireContraintes() { contraintesCheck(true); }

QStringList Article::valeursAutorisees(const QString &colonne)
{
    return contraintesCheck().value(colonne.toUpper());
}

QStringList Article::etats()
{
    const QStringList v = valeursAutorisees("ETAT");
    return v.isEmpty() ? QStringList{"Neuf", "Excellent", "Bon", "Moyen", "À restaurer"} : v;
}
QStringList Article::statuts()
{
    const QStringList v = valeursAutorisees("STATUT");
    return v.isEmpty() ? QStringList{"Disponible", "Réservé", "Vendu"} : v;
}
QStringList Article::categories()
{
    const QStringList v = valeursAutorisees("CATEGORIE");
    return v.isEmpty() ? QStringList{"Robe", "Costume", "Manteau", "Tailleur", "Veste", "Jupe", "Accessoire"} : v;
}
bool Article::categoriesFermees() { return !valeursAutorisees("CATEGORIE").isEmpty(); }
QStringList Article::maisons()    { return {"Chanel", "Dior", "Yves Saint Laurent", "Balenciaga", "Givenchy", "Balmain"}; }
QList<int> Article::decennies()   { return {1920, 1930, 1940, 1950, 1960, 1970, 1980, 1990, 2000}; }

// ----------------------------------------------------------------------------------------------------------
// Contrôle de saisie
// ----------------------------------------------------------------------------------------------------------

static bool contientLettre(const QString &s)
{
    for (const QChar &c : s) if (c.isLetter()) return true;
    return false;
}

static bool nombreValide(const QString &texte, double *out)
{
    QString t = texte.trimmed();
    t.replace(',', '.');
    static const QRegularExpression re(R"(^\d{1,7}(\.\d{1,3})?$)");
    if (!re.match(t).hasMatch()) return false;
    if (out) *out = t.toDouble();
    return true;
}

QString Article::verifierChamp(const QString &champ, const Saisie &s)
{
    static const QRegularExpression nomRe(QString::fromUtf8(R"(^[\p{L}\p{N}][\p{L}\p{N} '’\-«».,&/()]*$)"));
    static const QRegularExpression lettresRe(QString::fromUtf8(R"(^\p{L}[\p{L} '’\-.&]*$)"));
    static const QRegularExpression origineRe(QString::fromUtf8(R"(^[\p{L}\p{N}][\p{L}\p{N} '’\-«».,&/()]*$)"));

    if (champ == "nom") {
        const QString v = s.nom.simplified();
        if (v.isEmpty()) return "Le nom est obligatoire.";
        if (v.size() > 80) return "80 caractères maximum.";
        if (!contientLettre(v) || !nomRe.match(v).hasMatch()) return "Lettres obligatoires ; chiffres, - ' « » autorisés (pas de symboles).";
    } else if (champ == "maison") {
        const QString v = s.maison.simplified();
        if (v.isEmpty()) return "Choisissez ou saisissez une maison de couture.";
        if (v.size() > 60) return "60 caractères maximum.";
        if (!lettresRe.match(v).hasMatch()) return "La maison ne peut contenir que des lettres.";
    } else if (champ == "decennie") {
        bool ok = false;
        const int d = s.decennie.trimmed().toInt(&ok);
        if (!ok) return "Choisissez une décennie.";
        if (d < 1800 || d > QDate::currentDate().year()) return "Décennie invalide.";
    } else if (champ == "etat") {
        if (s.etat.isEmpty()) return "Choisissez l'état de l'article.";
        if (!etats().contains(s.etat)) return "État invalide.";
    } else if (champ == "prix") {
        double p = 0;
        if (s.prix.trimmed().isEmpty()) return "Le prix est obligatoire (ou utilisez l'estimation).";
        if (!nombreValide(s.prix, &p)) return "Nombre positif (3 décimales maximum).";
    } else if (champ == "categorie") {
        const QString v = s.categorie.simplified();
        if (v.isEmpty()) return "Choisissez ou saisissez une catégorie.";
        if (v.size() > 40) return "40 caractères maximum.";
        if (!lettresRe.match(v).hasMatch()) return "La catégorie ne peut contenir que des lettres.";
    } else if (champ == "origine") {
        const QString v = s.origine.simplified();
        if (v.isEmpty()) return "L'origine est obligatoire (provenance).";
        if (v.size() > 100) return "100 caractères maximum.";
        if (!contientLettre(v) || !origineRe.match(v).hasMatch()) return "Provenance : lettres, chiffres et , . - ' autorisés.";
    } else if (champ == "statut") {
        if (s.statut.isEmpty()) return "Choisissez un statut.";
        if (!statuts().contains(s.statut)) return "Statut invalide.";
    } else if (champ == "service") {
        if (s.idService <= 0) return "Choisissez le service associé.";
    } else if (champ == "date") {
        if (!s.date.isValid()) return "Date invalide.";
        if (s.date > QDate::currentDate()) return "La date ne peut pas être dans le futur.";
    }
    return QString();
}

QMap<QString, QString> Article::verifier(const Saisie &s, Article *out)
{
    QMap<QString, QString> e;
    for (const char *c : {"nom", "maison", "decennie", "etat", "prix", "categorie", "origine", "statut", "service", "date"}) {
        const QString m = verifierChamp(c, s);
        if (!m.isEmpty()) e[c] = m;
    }
    if (e.isEmpty() && out) {
        double p = 0;
        nombreValide(s.prix, &p);
        *out = Article(out->id, s.nom.simplified(), s.maison.simplified(), s.decennie.trimmed().toInt(), s.etat, p,
                       s.categorie.simplified(), s.origine.simplified(), s.statut, s.idService, s.date);
    }
    return e;
}

// ----------------------------------------------------------------------------------------------------------
// Métier innovant : estimation du prix d'un article vintage selon la maison, la décennie et l'état
// ----------------------------------------------------------------------------------------------------------

Article::Estimation Article::estimerPrix(const QString &maisonTexte, int decennie, const QString &etatTexte)
{
    Estimation e;
    e.base = 1000;
    if (maisonTexte.contains("Chanel", Qt::CaseInsensitive)) e.maison = 2500;
    else if (maisonTexte.contains("Dior", Qt::CaseInsensitive)) e.maison = 2000;
    else if (maisonTexte.contains("Yves", Qt::CaseInsensitive) || maisonTexte.contains("YSL", Qt::CaseInsensitive)) e.maison = 1800;
    else e.maison = 1000;

    if (decennie < 1940) e.decennie = 1800;
    else if (decennie < 1960) e.decennie = 1500;
    else if (decennie < 1970) e.decennie = 1200;
    else if (decennie < 1980) e.decennie = 900;
    else if (decennie < 1990) e.decennie = 600;
    else e.decennie = 300;

    if (etatTexte.contains("Neuf", Qt::CaseInsensitive)) e.etat = 1000;
    else if (etatTexte.contains("Excellent", Qt::CaseInsensitive)) e.etat = 750;
    else if (etatTexte.contains("Bon", Qt::CaseInsensitive)) e.etat = 400;
    else e.etat = 100;

    e.total = e.base + e.maison + e.decennie + e.etat;
    return e;
}
