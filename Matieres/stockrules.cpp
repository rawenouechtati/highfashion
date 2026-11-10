#include "stockrules.h"
#include <QRegularExpression>
#include <cmath>

namespace Mat {


namespace Stock {

QString statusOf(double q, double seuil)
{
    if (q <= 0) return Rupture;
    if (q <= seuil) return StockFaible;
    return Disponible;
}

int statusRank(const Material &m)
{
    const QString s = statusOf(m.quantite, m.seuil);
    return s == Rupture ? 0 : (s == StockFaible ? 1 : 2);
}

QString statusColor(const QString &s)
{
    return s == Rupture ? "#b3261e" : (s == StockFaible ? "#b86e00" : "#2e7d32");
}

QString statusTint(const QString &s)
{
    return s == Rupture ? "#f8dcd9" : (s == StockFaible ? "#fbebd0" : "#e0f0e1");
}

double valueOf(const Material &m) { return m.quantite * m.prix; }

QStringList defaultUnits()
{
    return {"metre", "bobine", "piece"};   // CK_MP_UNITE de la base v5
}

bool isIntUnit(const QString &u)
{
    static const QStringList ints = {"piece", "bobine"};
    return ints.contains(u);
}

static QString group(const QString &digits)
{
    QString out;
    const int n = digits.size();
    for (int i = 0; i < n; ++i) {
        out += digits[i];
        const int rest = n - i - 1;
        if (rest > 0 && rest % 3 == 0) out += ' ';
    }
    return out;
}

QString fmtNum(double n)
{
    const bool neg = n < 0;
    const double r = std::round(std::fabs(n) * 100.0) / 100.0;
    const QStringList p = QString::number(r, 'f', 2).split('.');
    QString dec = p.value(1);
    while (dec.endsWith('0')) dec.chop(1);
    QString res = group(p[0]) + (dec.isEmpty() ? QString() : "," + dec);
    return neg ? "-" + res : res;
}

QString fmtMoney(double n)
{
    const bool neg = n < 0;
    const QStringList p = QString::number(std::fabs(n), 'f', 3).split('.');
    const QString res = group(p[0]) + "," + p[1];
    return neg ? "-" + res : res;
}

QString fmtDate(const QDate &d) { return d.toString("dd/MM/yyyy"); }

QString normalize(const QString &s)
{
    const QString d = s.normalized(QString::NormalizationForm_D);
    QString out;
    for (const QChar &c : d)
        if (c.category() != QChar::Mark_NonSpacing) out += c;
    return out.toLower().trimmed();
}

bool parseNum(const QString &text, double *out)
{
    QString s = text.trimmed();
    s.replace(',', '.');
    s.remove(' ');
    if (s.isEmpty()) return false;
    bool ok = false;
    const double v = s.toDouble(&ok);
    if (!ok || !std::isfinite(v)) return false;
    if (out) *out = v;
    return true;
}

static bool checkNum(const QString &raw, const QString &required, const QString &negative, bool integer,
                     const QString &key, Errors &e, double *out)
{
    double v = 0;
    if (!parseNum(raw, &v)) { e[key] = required; return false; }
    if (v < 0) { e[key] = negative; return false; }
    if (v > 1e9) { e[key] = "Valeur trop grande."; return false; }
    if (integer && v != std::floor(v)) { e[key] = "Cette unité n'accepte que des quantités entières."; return false; }
    *out = v;
    return true;
}

bool isValidName(const QString &text)
{
    // lettres (accents compris) ; espace, tiret ou apostrophe uniquement ENTRE deux mots
    static const QRegularExpression re("^\\p{L}+(?:[ '\\x{2019}\\-]\\p{L}+)*$");
    return re.match(text).hasMatch();
}

Errors validateMaterial(const RawMaterial &raw, bool nameUsed, Material *out)
{
    Errors e;
    Material m;
    m.nom = raw.nom.simplified();   // espaces superflus supprimés
    if (m.nom.isEmpty()) e["nom"] = "Le nom est obligatoire.";
    else if (m.nom.size() > 80) e["nom"] = "80 caractères maximum.";
    else if (!isValidName(m.nom)) e["nom"] = "Le nom ne peut contenir que des lettres (pas de chiffres ni de symboles).";
    else if (nameUsed) e["nom"] = "Cette matière existe déjà : le nom doit être unique.";

    m.categorie = raw.categorie.simplified();
    if (m.categorie.isEmpty()) e["categorie"] = "Choisissez ou saisissez une catégorie.";
    else if (m.categorie.size() > 40) e["categorie"] = "40 caractères maximum.";
    else if (!isValidName(m.categorie)) e["categorie"] = "La catégorie ne peut contenir que des lettres.";

    m.fournisseur = raw.fournisseur.simplified();
    if (m.fournisseur.isEmpty()) e["fournisseur"] = "Choisissez ou saisissez un fournisseur.";
    else if (m.fournisseur.size() > 60) e["fournisseur"] = "60 caractères maximum.";
    else if (!isValidName(m.fournisseur)) e["fournisseur"] = "Le fournisseur ne peut contenir que des lettres.";

    m.unite = raw.unite.trimmed();
    if (m.unite.isEmpty()) e["unite"] = "Choisissez une unité.";
    else if (!defaultUnits().contains(m.unite)) e["unite"] = "Unité invalide : metre, bobine ou piece (contrainte de la base).";

    checkNum(raw.quantite, "La quantité est obligatoire (nombre).", "La quantité ne peut pas être négative.",
             isIntUnit(m.unite), "quantite", e, &m.quantite);
    checkNum(raw.seuil, "Le seuil d'alerte est obligatoire (nombre).", "Le seuil ne peut pas être négatif.",
             false, "seuil", e, &m.seuil);
    checkNum(raw.prix, "Le prix unitaire est obligatoire (nombre).", "Le prix ne peut pas être négatif.",
             false, "prix", e, &m.prix);

    if (out) *out = m;
    return e;
}

Estimate estimate(const Material &m, double days, bool manual, double manualQty, const QString &per,
                  double historySum, int windowDays)
{
    Estimate r;
    r.material = m;
    r.manual = manual;
    if (!(days > 0)) { r.error = "Saisissez une période valide (supérieure à zéro)."; return r; }
    double daily = 0;
    if (manual) {
        if (manualQty < 0) { r.error = "Saisissez une consommation valide (nombre positif ou nul)."; return r; }
        const double div = per == "semaine" ? 7.0 : (per == "mois" ? 30.0 : 1.0);
        daily = manualQty / div;
    } else {
        if (historySum <= 0) {
            r.noHistory = true;
            r.error = QString("Aucune sortie enregistrée sur les %1 derniers jours pour cette matière. "
                              "Utilisez la saisie manuelle.").arg(windowDays);
            return r;
        }
        daily = historySum / windowDays;
        r.sum = historySum;
        r.windowDays = windowDays;
    }
    r.ok = true;
    r.daily = daily;
    r.days = days;
    r.need = daily * days;
    r.stock = m.quantite;
    r.diff = m.quantite - r.need;
    r.sufficient = r.diff >= 0;
    r.autonomy = daily > 0 ? m.quantite / daily : INFINITY;
    return r;
}

}  // namespace Stock

}  // namespace Mat
