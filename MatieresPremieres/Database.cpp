#include "Database.h"
#include "connection.h"
#include <QDebug>
#include <QRegularExpression>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <algorithm>
#include <cmath>

static const QStringList kDefaultCategories = {"Tissu", "Fil", "Bouton", "Fermeture", "Doublure"};

// Les paramètres de connexion Oracle (DSN, utilisateur, mot de passe) sont dans la classe Connection.

static double round3(double v) { return std::round(v * 1000.0) / 1000.0; }

Database::Database(QObject *parent) : QObject(parent)
{
    conn_ = QString("gmp_%1").arg(reinterpret_cast<quintptr>(this));
}

Database::~Database()
{
    if (db_.isValid()) {
        if (db_.isOpen()) db_.close();
        db_ = QSqlDatabase();
        QSqlDatabase::removeDatabase(conn_);
    }
}

void Database::noteError(const QSqlQuery &q, const QString &sql) const
{
    lastError_ = q.lastError().text();
    qWarning().noquote() << "Erreur SQL :" << lastError_ << "|" << sql;
}

bool Database::run(QSqlQuery &q, const QString &sql) const
{
    const bool ok = sql.isEmpty() ? q.exec() : q.exec(sql);
    if (ok) lastError_.clear();
    else noteError(q, sql.isEmpty() ? q.lastQuery() : sql);
    return ok;
}

// Requête préparée : toutes les valeurs passent par des paramètres liés (jamais concaténées).
bool Database::exec(const QString &sql, const QVariantList &args) const
{
    QSqlQuery q(db_);
    if (!q.prepare(sql)) { noteError(q, sql); return false; }
    for (const QVariant &a : args) q.addBindValue(a);
    return run(q);
}

// Remplace (supprime puis insère) une ligne de la table meta : portable Oracle / SQLite.
bool Database::setMeta(const QString &key, const QString &value)
{
    return exec("DELETE FROM meta WHERE cle=?", {key}) && exec("INSERT INTO meta(cle,valeur) VALUES(?,?)", {key, value});
}

QString Database::defaultTarget() { return QStringLiteral("odbc:") + Connection::defaultDsn(); }

bool Database::open(const QString &target, QString *error)
{
    // ré-ouverture (ex. repli sur une base temporaire après un échec Oracle)
    if (db_.isValid() && db_.isOpen()) db_.close();
    db_ = QSqlDatabase();
    if (QSqlDatabase::contains(conn_)) QSqlDatabase::removeDatabase(conn_);
    oracle_ = target.startsWith("odbc", Qt::CaseInsensitive);
    if (oracle_) {
        // « odbc:NOM_DSN » ou simplement « odbc » (DSN de l'équipe, défini dans Connection)
        const QString dsn = target.contains(':') ? target.section(':', 1).trimmed() : QString();
        db_ = Connection::create(conn_, dsn);
    } else {
        db_ = QSqlDatabase::addDatabase("QSQLITE", conn_);
        db_.setDatabaseName(target);
    }
    if (!db_.open()) {
        if (error) *error = db_.lastError().text();
        qWarning().noquote() << "Connexion impossible :" << db_.lastError().text();
        return false;
    }
    if (!createTables()) {
        if (error) *error = oracle_
            ? "Tables Oracle introuvables ou ancienne structure : exécutez le script gestion_materiel_oracle.sql, puis (base de la v1.7) migration_oracle_v1.8_nom_cle_primaire.sql dans le schéma de l'utilisateur.\n" + lastError_
            : QString("Impossible de créer les tables de la base de données.");
        return false;
    }
    // Premier lancement : on charge les données de démonstration (le script Oracle pose déjà « initialised »).
    QSqlQuery q(db_);
    run(q, "SELECT valeur FROM meta WHERE cle='initialised'");
    if (!q.next()) {
        loadDemo();
        setMeta("initialised", "1");
    }
    syncAlerts();
    return true;
}

bool Database::createTables()
{
    if (oracle_) {
        // Oracle : le schéma est créé une fois pour toutes par gestion_materiel_oracle.sql.
        // On vérifie seulement que les tables sont présentes (et accessibles) pour l'utilisateur connecté.
        for (const char *t : {"categories", "suppliers", "materials", "movements", "alerts", "meta"}) {
            QSqlQuery q(db_);
            if (!run(q, QString("SELECT COUNT(*) FROM %1").arg(t))) return false;
        }
        // v1.8 : le nom est la clé primaire. Une base de la v1.7 (colonnes ID / FOURNISSEUR_ID) est refusée ici.
        QSqlQuery q(db_);
        if (!run(q, "SELECT nom, categorie, fournisseur FROM materials WHERE 1=0")) return false;
        if (!run(q, "SELECT nom FROM suppliers WHERE 1=0")) return false;
        return true;
    }
    // SQLite (tests / base temporaire de secours)
    const QStringList ddl = {
        "CREATE TABLE IF NOT EXISTS categories(nom TEXT PRIMARY KEY COLLATE NOCASE)",
        "CREATE TABLE IF NOT EXISTS suppliers(nom TEXT PRIMARY KEY COLLATE NOCASE, contact TEXT, telephone TEXT, email TEXT)",
        "CREATE TABLE IF NOT EXISTS materials(nom TEXT PRIMARY KEY COLLATE NOCASE, "
        "categorie TEXT NOT NULL, fournisseur TEXT NOT NULL, quantite REAL NOT NULL CHECK(quantite>=0), "
        "unite TEXT NOT NULL, seuil REAL NOT NULL CHECK(seuil>=0), prix REAL NOT NULL CHECK(prix>=0))",
        "CREATE TABLE IF NOT EXISTS movements(id INTEGER PRIMARY KEY AUTOINCREMENT, mov_date TEXT NOT NULL, "
        "material_id TEXT NOT NULL, nom TEXT, unite TEXT, type TEXT NOT NULL CHECK(type IN ('Entrée','Sortie')), "
        "quantite REAL NOT NULL CHECK(quantite>0), description TEXT)",
        "CREATE TABLE IF NOT EXISTS alerts(material_id TEXT PRIMARY KEY COLLATE NOCASE, nom TEXT, statut TEXT, "
        "quantite REAL, unite TEXT, seuil REAL, signature TEXT, alert_date TEXT)",
        "CREATE TABLE IF NOT EXISTS meta(cle TEXT PRIMARY KEY, valeur TEXT)"};
    for (const QString &s : ddl)
        if (!exec(s)) return false;
    return true;
}

void Database::finishChange()
{
    const QVector<Alert> created = syncAlerts();
    emit changed();
    if (!created.isEmpty()) emit alertsCreated(created);
}

// ============================ matières ============================
static Material readMaterial(QSqlQuery &q)
{
    Material m;
    m.nom = q.value(0).toString();
    m.categorie = q.value(1).toString();
    m.fournisseur = q.value(2).toString();
    m.quantite = q.value(3).toDouble();
    m.unite = q.value(4).toString();
    m.seuil = q.value(5).toDouble();
    m.prix = q.value(6).toDouble();
    return m;
}

QVector<Material> Database::materials() const
{
    QVector<Material> v;
    QSqlQuery q(db_);
    run(q, "SELECT nom,categorie,fournisseur,quantite,unite,seuil,prix FROM materials ORDER BY LOWER(nom)");
    while (q.next()) v.push_back(readMaterial(q));
    return v;
}

bool Database::getMaterial(const QString &nom, Material *out) const
{
    QSqlQuery q(db_);
    q.prepare("SELECT nom,categorie,fournisseur,quantite,unite,seuil,prix FROM materials WHERE nom=?");
    q.addBindValue(nom);
    if (!run(q) || !q.next()) return false;
    if (out) *out = readMaterial(q);
    return true;
}

bool Database::materialExists(const QString &nom) const { return getMaterial(nom, nullptr); }

// ============================ catégories ============================
QStringList Database::categories() const
{
    QStringList l;
    QSqlQuery q(db_);
    run(q, "SELECT nom FROM categories ORDER BY " + orderColumn());
    while (q.next()) l << q.value(0).toString();
    return l;
}

int Database::categoryUsage(const QString &name) const
{
    QSqlQuery q(db_);
    q.prepare("SELECT COUNT(*) FROM materials WHERE categorie=?");
    q.addBindValue(name);
    return (run(q) && q.next()) ? q.value(0).toInt() : 0;
}

bool Database::addCategory(const QString &name)
{
    const bool ok = exec("INSERT INTO categories(nom) VALUES(?)", {name.simplified()});
    if (ok) finishChange();
    return ok;
}

bool Database::renameCategory(const QString &oldName, const QString &newName)
{
    if (!db_.transaction()) return false;
    const bool ok = exec("UPDATE categories SET nom=? WHERE nom=?", {newName.simplified(), oldName}) &&
                    exec("UPDATE materials SET categorie=? WHERE categorie=?", {newName.simplified(), oldName});
    if (!ok) { db_.rollback(); return false; }
    if (!db_.commit()) { lastError_ = db_.lastError().text(); db_.rollback(); return false; }
    finishChange();
    return true;
}

bool Database::deleteCategory(const QString &name)
{
    if (categoryUsage(name) > 0) return false;
    const bool ok = exec("DELETE FROM categories WHERE nom=?", {name});
    if (ok) finishChange();
    return ok;
}

// ============================ fournisseurs ============================
QVector<Supplier> Database::suppliers() const
{
    QVector<Supplier> v;
    QSqlQuery q(db_);
    run(q, "SELECT nom,contact,telephone,email FROM suppliers ORDER BY LOWER(nom)");
    while (q.next())
        v.push_back({q.value(0).toString(), q.value(1).toString(), q.value(2).toString(), q.value(3).toString()});
    return v;
}

QStringList Database::supplierNames() const
{
    QStringList l;
    for (const Supplier &s : suppliers()) l << s.nom;
    return l;
}

int Database::supplierUsage(const QString &name) const
{
    QSqlQuery q(db_);
    q.prepare("SELECT COUNT(*) FROM materials WHERE fournisseur=?");
    q.addBindValue(name);
    return (run(q) && q.next()) ? q.value(0).toInt() : 0;
}

// ============================ mouvements ============================
QVector<Movement> Database::movements() const
{
    QVector<Movement> v;
    QSqlQuery q(db_);
    run(q, "SELECT id,mov_date,material_id,nom,unite,type,quantite,description FROM movements ORDER BY mov_date DESC, id DESC");
    while (q.next()) {
        Movement m;
        m.id = q.value(0).toInt();
        m.date = q.value(1).toDate();   // DATE Oracle ou texte ISO SQLite
        m.materialId = q.value(2).toString();
        m.nom = q.value(3).toString();
        m.unite = q.value(4).toString();
        m.type = q.value(5).toString();
        m.quantite = q.value(6).toDouble();
        m.description = q.value(7).toString();
        v.push_back(m);
    }
    return v;
}

Stock::Errors Database::recordMovement(const QString &materialNom, const QString &type, const QString &qtyText,
                                       const QDate &date, const QString &description, Movement *out)
{
    Stock::Errors e;
    Material m;
    const bool found = getMaterial(materialNom, &m);
    if (!found) e["materialId"] = "Choisissez une matière première.";
    if (type != "Entrée" && type != "Sortie") e["type"] = "Type de mouvement invalide.";

    double q = 0;
    if (!Stock::parseNum(qtyText, &q)) e["quantite"] = "La quantité est obligatoire (nombre).";
    else if (q <= 0) e["quantite"] = "La quantité doit être supérieure à zéro.";
    else if (found && Stock::isIntUnit(m.unite) && q != std::floor(q))
        e["quantite"] = "Cette unité n'accepte que des quantités entières.";
    else if (found && type == "Sortie" && q > m.quantite + 1e-9)   // jamais de stock négatif
        e["quantite"] = QString("Stock insuffisant : disponible %1 %2.").arg(Stock::fmtNum(m.quantite), m.unite);

    if (!date.isValid()) e["date"] = "Date invalide.";
    else if (date > QDate::currentDate()) e["date"] = "La date ne peut pas être dans le futur.";
    if (description.trimmed().size() > 200) e["description"] = "200 caractères maximum.";
    if (!e.isEmpty()) return e;

    const double newQty = round3(type == "Entrée" ? m.quantite + q : m.quantite - q);
    if (!db_.transaction()) { e["quantite"] = "Erreur de base de données."; return e; }
    const bool ok = exec("UPDATE materials SET quantite=? WHERE nom=?", {newQty, m.nom}) &&
                    exec("INSERT INTO movements(mov_date,material_id,nom,unite,type,quantite,description) VALUES(?,?,?,?,?,?,?)",
                         {date, m.nom, m.nom, m.unite, type, q, description.trimmed()});   // material_id = nom de la matière ; id du mouvement : séquence + trigger (Oracle)
    if (!ok) { db_.rollback(); e["quantite"] = "Erreur de base de données."; return e; }
    db_.commit();
    if (out) {
        out->date = date; out->materialId = m.nom; out->nom = m.nom; out->unite = m.unite;
        out->type = type; out->quantite = q; out->description = description.trimmed();
    }
    finishChange();
    return e;
}

double Database::consumedSum(const QString &materialId, int windowDays) const
{
    const QDate today = QDate::currentDate();
    QSqlQuery q(db_);
    q.prepare("SELECT COALESCE(SUM(quantite),0) FROM movements WHERE type='Sortie' AND material_id=? AND mov_date>=? AND mov_date<=?");
    q.addBindValue(materialId);
    q.addBindValue(today.addDays(-(windowDays - 1)));
    q.addBindValue(today);
    return (run(q) && q.next()) ? round3(q.value(0).toDouble()) : 0.0;
}

// ============================ alertes ============================
// Une alerte par matière en alerte. Sa « signature » (statut|quantité|seuil)
// décrit la situation : tant qu'elle ne change pas, aucune nouvelle alerte
// n'est créée. Si la situation change, l'alerte est remplacée. Si la matière
// sort de l'alerte, l'alerte disparaît.
QVector<Alert> Database::syncAlerts()
{
    QVector<Alert> created;
    QSet<QString> keep;
    for (const Material &m : materials()) {
        const QString st = Stock::statusOf(m.quantite, m.seuil);
        if (st == Stock::Disponible) continue;
        const QString sig = QString("%1|%2|%3").arg(st).arg(m.quantite, 0, 'g', 12).arg(m.seuil, 0, 'g', 12);
        keep.insert(m.nom.toLower());
        QSqlQuery q(db_);
        q.prepare("SELECT signature FROM alerts WHERE material_id=?");
        q.addBindValue(m.nom);
        if (run(q) && q.next() && q.value(0).toString() == sig) continue;   // inchangée : pas de doublon
        exec("DELETE FROM alerts WHERE material_id=?", {m.nom});   // remplace l'alerte précédente (portable Oracle / SQLite)
        exec("INSERT INTO alerts(material_id,nom,statut,quantite,unite,seuil,signature,alert_date) VALUES(?,?,?,?,?,?,?,?)",
             {m.nom, m.nom, st, m.quantite, m.unite, m.seuil, sig, QDate::currentDate()});
        Alert a;
        a.materialId = m.nom; a.nom = m.nom; a.statut = st; a.unite = m.unite;
        a.quantite = m.quantite; a.seuil = m.seuil; a.signature = sig; a.date = QDate::currentDate();
        created.push_back(a);
    }
    // supprime les alertes des matières qui ne sont plus en alerte
    QStringList stale;
    QSqlQuery q(db_);
    run(q, "SELECT material_id FROM alerts");
    while (q.next())
        if (!keep.contains(q.value(0).toString().toLower())) stale << q.value(0).toString();
    for (const QString &id : stale) exec("DELETE FROM alerts WHERE material_id=?", {id});
    return created;
}

QVector<Alert> Database::alerts() const
{
    QVector<Alert> v;
    QSqlQuery q(db_);
    run(q, "SELECT material_id,nom,statut,quantite,unite,seuil,signature,alert_date FROM alerts "
           "ORDER BY CASE statut WHEN 'Rupture' THEN 0 ELSE 1 END, nom");
    while (q.next()) {
        Alert a;
        a.materialId = q.value(0).toString(); a.nom = q.value(1).toString(); a.statut = q.value(2).toString();
        a.quantite = q.value(3).toDouble(); a.unite = q.value(4).toString(); a.seuil = q.value(5).toDouble();
        a.signature = q.value(6).toString(); a.date = q.value(7).toDate();
        v.push_back(a);
    }
    return v;
}

int Database::alertCount() const
{
    QSqlQuery q(db_);
    return (run(q, "SELECT COUNT(*) FROM alerts") && q.next()) ? q.value(0).toInt() : 0;
}

void Database::refreshAlerts() { finishChange(); }

// ============================ statistiques ============================
Stats Database::stats() const
{
    Stats s;
    QMap<QString, CategoryStat> cats;
    for (const QString &c : categories()) { CategoryStat cs; cs.categorie = c; cats.insert(c, cs); }
    const QStringList order = categories();
    const QVector<Material> mats = materials();
    s.refs = mats.size();
    for (const Material &m : mats) {
        const QString st = Stock::statusOf(m.quantite, m.seuil);
        s.byUnit[m.unite] = round3(s.byUnit.value(m.unite) + m.quantite);
        if (st == Stock::StockFaible) s.low++;
        if (st == Stock::Rupture) s.out++;
        s.totalValue += Stock::valueOf(m);
        CategoryStat &c = cats[m.categorie];
        c.categorie = m.categorie;
        c.refs++;
        c.value += Stock::valueOf(m);
        c.units[m.unite] = round3(c.units.value(m.unite) + m.quantite);
        if (st != Stock::Disponible) s.replenish.push_back(m);
    }
    for (const QString &name : order) s.byCategory.push_back(cats.take(name));
    for (const CategoryStat &c : cats) s.byCategory.push_back(c);   // catégories orphelines éventuelles
    std::sort(s.replenish.begin(), s.replenish.end(), [](const Material &a, const Material &b) {
        if (Stock::statusRank(a) != Stock::statusRank(b)) return Stock::statusRank(a) < Stock::statusRank(b);
        return (a.seuil - a.quantite) > (b.seuil - b.quantite);
    });
    return s;
}

// ============================ jeux de données ============================
bool Database::isDemo() const
{
    QSqlQuery q(db_);
    return run(q, "SELECT valeur FROM meta WHERE cle='demo'") && q.next() && q.value(0).toString() == "1";
}

void Database::insertDefaultCategories()
{
    const QStringList have = categories();
    for (const QString &c : kDefaultCategories)
        if (!have.contains(c, Qt::CaseInsensitive)) exec("INSERT INTO categories(nom) VALUES(?)", {c});
}

void Database::clearAll()
{
    db_.transaction();
    for (const char *t : {"materials", "suppliers", "movements", "alerts", "categories"})
        exec(QString("DELETE FROM %1").arg(t));
    insertDefaultCategories();
    setMeta("demo", "0");
    db_.commit();
    finishChange();
}

// Données de DÉMONSTRATION uniquement : fournisseurs et stocks fictifs.
void Database::loadDemo()
{
    db_.transaction();
    for (const char *t : {"materials", "suppliers", "movements", "alerts", "categories"})
        exec(QString("DELETE FROM %1").arg(t));
    insertDefaultCategories();
    // Les noms (clés primaires) ne contiennent que des lettres : pas de chiffres dans les données de démonstration.
    for (const char *s : {"Fournisseur démo A", "Fournisseur démo B", "Fournisseur démo C"})
        exec("INSERT INTO suppliers(nom,contact,telephone,email) VALUES(?, '', '', '')", {QString::fromUtf8(s)});

    struct M { const char *nom, *cat, *sup; double q; const char *u; double seuil, prix; };
    const M mats[] = {
        {"Tissu coton blanc", "Tissu", "Fournisseur démo A", 120, "mètre", 30, 8.5},
        {"Tissu lin naturel", "Tissu", "Fournisseur démo B", 18, "mètre", 25, 14},
        {"Fil polyester noir", "Fil", "Fournisseur démo A", 45, "bobine", 10, 2.25},
        {"Fil coton blanc", "Fil", "Fournisseur démo A", 8, "bobine", 10, 2},
        {"Bouton quatre trous", "Bouton", "Fournisseur démo C", 500, "pièce", 100, 0.12},
        {"Bouton métal", "Bouton", "Fournisseur démo C", 0, "pièce", 80, 0.35},
        {"Fermeture éclair courte", "Fermeture", "Fournisseur démo B", 150, "pièce", 40, 0.9},
        {"Fermeture invisible longue", "Fermeture", "Fournisseur démo B", 35, "pièce", 35, 1.2},
        {"Doublure satin", "Doublure", "Fournisseur démo A", 80, "mètre", 20, 5.75},
        {"Doublure viscose", "Doublure", "Fournisseur démo C", 12, "mètre", 20, 6.4}};
    for (const M &m : mats)
        exec("INSERT INTO materials(nom,categorie,fournisseur,quantite,unite,seuil,prix) VALUES(?,?,?,?,?,?,?)",
             {QString::fromUtf8(m.nom), QString::fromUtf8(m.cat), QString::fromUtf8(m.sup), m.q, QString::fromUtf8(m.u), m.seuil, m.prix});

    struct H { const char *nom; int daysAgo; const char *type; double q; };
    const H hist[] = {
        {"Tissu coton blanc", 70, "Entrée", 250}, {"Tissu coton blanc", 55, "Sortie", 40}, {"Tissu coton blanc", 38, "Sortie", 35},
        {"Tissu coton blanc", 21, "Sortie", 30}, {"Tissu coton blanc", 9, "Sortie", 25}, {"Tissu lin naturel", 60, "Entrée", 100},
        {"Tissu lin naturel", 45, "Sortie", 30}, {"Tissu lin naturel", 30, "Sortie", 28}, {"Tissu lin naturel", 14, "Sortie", 24},
        {"Fil polyester noir", 50, "Sortie", 6}, {"Fil polyester noir", 25, "Sortie", 8}, {"Fil polyester noir", 6, "Sortie", 7},
        {"Fil coton blanc", 40, "Sortie", 9}, {"Fil coton blanc", 18, "Sortie", 8}, {"Fil coton blanc", 4, "Sortie", 6},
        {"Bouton quatre trous", 33, "Sortie", 120}, {"Bouton quatre trous", 12, "Sortie", 100},
        {"Bouton métal", 28, "Sortie", 90}, {"Bouton métal", 3, "Sortie", 60}};
    for (const H &h : hist) {
        Material m;
        getMaterial(QString::fromUtf8(h.nom), &m);
        exec("INSERT INTO movements(mov_date,material_id,nom,unite,type,quantite,description) VALUES(?,?,?,?,?,?,?)",
             {QDate::currentDate().addDays(-h.daysAgo), m.nom, m.nom, m.unite, QString::fromUtf8(h.type), h.q,
              QString("Mouvement de démonstration")});
    }
    setMeta("demo", "1");
    db_.commit();
    finishChange();
}
