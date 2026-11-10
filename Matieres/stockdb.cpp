#include "stockdb.h"
#include "matiere.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QStandardPaths>
#include <algorithm>
#include <cmath>

namespace Mat {

static const QStringList kDefaultCategories = {"Tissu", "Fil", "Bouton", "Fermeture", "Doublure"};

static double round3(double v) { return std::round(v * 1000.0) / 1000.0; }

StockDb::StockDb(QObject *parent) : QObject(parent)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!dir.isEmpty()) historyFile_ = QDir(dir).filePath("mouvements_matieres.json");
}

void StockDb::setHistoryFile(const QString &path)
{
    historyFile_ = path;
    history_.clear();
    nextMovementId_ = 1;
    loadHistory();
}

bool StockDb::charger()
{
    lastError_.clear();
    QString err;
    if (!Matiere::tableAccessible(&err)) {
        lastError_ = err;
        return false;
    }
    loadHistory();
    materials_ = Matiere::tous(&err);
    if (!err.isEmpty()) { lastError_ = err; return false; }
    syncAlerts();    // état de départ : les alertes déjà présentes ne déclenchent pas de notification
    emit changed();
    return true;
}

void StockDb::recharger()
{
    QString err;
    const QVector<Material> fresh = Matiere::tous(&err);
    if (err.isEmpty()) materials_ = fresh;
    else lastError_ = err;
    const QVector<Alert> created = syncAlerts();
    emit changed();
    if (!created.isEmpty()) emit alertsCreated(created);
}

// ============================ matières ============================
bool StockDb::getMaterial(const QString &nom, Material *out) const
{
    for (const Material &m : materials_)
        if (m.nom == nom) { if (out) *out = m; return true; }
    for (const Material &m : materials_)   // repli : sans tenir compte de la casse
        if (m.nom.compare(nom, Qt::CaseInsensitive) == 0) { if (out) *out = m; return true; }
    return false;
}

QStringList StockDb::categories() const
{
    QStringList l = kDefaultCategories;
    QStringList extra;
    for (const Material &m : materials_)
        if (!m.categorie.isEmpty() && !l.contains(m.categorie, Qt::CaseInsensitive)
            && !extra.contains(m.categorie, Qt::CaseInsensitive))
            extra << m.categorie;
    extra.sort(Qt::CaseInsensitive);
    return l + extra;
}

QStringList StockDb::supplierNames() const
{
    QStringList l;
    for (const Material &m : materials_)
        if (!m.fournisseur.isEmpty() && !l.contains(m.fournisseur, Qt::CaseInsensitive)) l << m.fournisseur;
    l.sort(Qt::CaseInsensitive);
    return l;
}

void StockDb::materialRenamed(const QString &oldName, const QString &newName)
{
    if (oldName == newName) return;
    bool touched = false;
    for (Movement &mv : history_)
        if (mv.materialId == oldName) { mv.materialId = newName; mv.nom = newName; touched = true; }
    if (touched) saveHistory();
}

// ============================ mouvements ============================
QVector<Movement> StockDb::movements() const
{
    QVector<Movement> v = history_;
    std::stable_sort(v.begin(), v.end(), [](const Movement &a, const Movement &b) {
        if (a.date != b.date) return a.date > b.date;
        return a.id > b.id;
    });
    return v;
}

Stock::Errors StockDb::recordMovement(const QString &materialNom, const QString &type, const QString &qtyText,
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
    QString err;
    if (!Matiere::changerQuantite(m.id, newQty, &err)) {
        lastError_ = err;
        e["quantite"] = "Erreur de base de données : " + err;
        return e;
    }
    Movement mv;
    mv.id = nextMovementId_++;
    mv.date = date; mv.materialId = m.nom; mv.nom = m.nom; mv.unite = m.unite;
    mv.type = type; mv.quantite = q; mv.description = description.trimmed();
    history_.push_back(mv);
    saveHistory();
    if (out) *out = mv;
    recharger();
    return e;
}

double StockDb::consumedSum(const QString &materialId, int windowDays) const
{
    const QDate today = QDate::currentDate();
    const QDate from = today.addDays(-(windowDays - 1));
    double sum = 0;
    for (const Movement &mv : history_)
        if (mv.type == "Sortie" && mv.materialId == materialId && mv.date >= from && mv.date <= today) sum += mv.quantite;
    return round3(sum);
}

// ---- historique : fichier JSON local ----
void StockDb::loadHistory()
{
    history_.clear();
    nextMovementId_ = 1;
    if (historyFile_.isEmpty()) return;
    QFile f(historyFile_);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    for (const QJsonValue &v : doc.array()) {
        const QJsonObject o = v.toObject();
        Movement mv;
        mv.id = o.value("id").toInt();
        mv.date = QDate::fromString(o.value("date").toString(), Qt::ISODate);
        mv.materialId = o.value("nom").toString();
        mv.nom = mv.materialId;
        mv.unite = o.value("unite").toString();
        mv.type = o.value("type").toString();
        mv.quantite = o.value("quantite").toDouble();
        mv.description = o.value("description").toString();
        if (mv.date.isValid() && !mv.materialId.isEmpty()) {
            history_.push_back(mv);
            nextMovementId_ = std::max(nextMovementId_, mv.id + 1);
        }
    }
}

void StockDb::saveHistory() const
{
    if (historyFile_.isEmpty()) return;
    QDir().mkpath(QFileInfo(historyFile_).absolutePath());
    QJsonArray arr;
    for (const Movement &mv : history_) {
        QJsonObject o;
        o["id"] = mv.id;
        o["date"] = mv.date.toString(Qt::ISODate);
        o["nom"] = mv.materialId;
        o["unite"] = mv.unite;
        o["type"] = mv.type;
        o["quantite"] = mv.quantite;
        o["description"] = mv.description;
        arr.append(o);
    }
    QFile f(historyFile_);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

// ============================ alertes ============================
// Une alerte par matière en alerte. Sa « signature » (statut|quantité|seuil) décrit la situation :
// tant qu'elle ne change pas, aucune nouvelle alerte n'est créée. Si la situation change, l'alerte
// est remplacée (et signalée). Si la matière sort de l'alerte, l'alerte disparaît.
QVector<Alert> StockDb::syncAlerts()
{
    QVector<Alert> created;
    QSet<QString> keep;
    for (const Material &m : materials_) {
        const QString st = Stock::statusOf(m.quantite, m.seuil);
        if (st == Stock::Disponible) continue;
        const QString sig = QString("%1|%2|%3").arg(st).arg(m.quantite, 0, 'g', 12).arg(m.seuil, 0, 'g', 12);
        const QString key = m.nom.toLower();
        keep.insert(key);
        const auto it = alerts_.constFind(key);
        if (it != alerts_.constEnd() && it->signature == sig) continue;   // inchangée : pas de doublon
        Alert a;
        a.materialId = m.nom; a.nom = m.nom; a.statut = st; a.unite = m.unite;
        a.quantite = m.quantite; a.seuil = m.seuil; a.signature = sig; a.date = QDate::currentDate();
        alerts_.insert(key, a);
        created.push_back(a);
    }
    for (auto it = alerts_.begin(); it != alerts_.end();) {
        if (!keep.contains(it.key())) it = alerts_.erase(it);
        else ++it;
    }
    return created;
}

QVector<Alert> StockDb::alerts() const
{
    QVector<Alert> v = alerts_.values().toVector();
    std::sort(v.begin(), v.end(), [](const Alert &a, const Alert &b) {
        const int ra = a.statut == Stock::Rupture ? 0 : 1, rb = b.statut == Stock::Rupture ? 0 : 1;
        if (ra != rb) return ra < rb;
        return a.nom.toLower() < b.nom.toLower();
    });
    return v;
}

// ============================ statistiques ============================
Stats StockDb::stats() const
{
    Stats s;
    QMap<QString, CategoryStat> cats;
    const QStringList order = categories();
    for (const QString &c : order) { CategoryStat cs; cs.categorie = c; cats.insert(c, cs); }
    s.refs = materials_.size();
    for (const Material &m : materials_) {
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
    for (const CategoryStat &c : cats) s.byCategory.push_back(c);   // catégories hors liste éventuelles
    std::sort(s.replenish.begin(), s.replenish.end(), [](const Material &a, const Material &b) {
        if (Stock::statusRank(a) != Stock::statusRank(b)) return Stock::statusRank(a) < Stock::statusRank(b);
        return (a.seuil - a.quantite) > (b.seuil - b.quantite);
    });
    return s;
}

}  // namespace Mat
