// Stock.h — règles métier pures (aucun accès à la base ni à l'interface)
#pragma once
#include "Models.h"
#include <QMap>
#include <QStringList>

namespace Stock {

using Errors = QMap<QString, QString>;   // champ -> message d'erreur

inline const QString Disponible = QStringLiteral("Disponible");
inline const QString StockFaible = QStringLiteral("Stock faible");
inline const QString Rupture = QStringLiteral("Rupture");

// --- statut : quantité > seuil -> Disponible ; 0 < quantité <= seuil -> Stock faible ; 0 -> Rupture
QString statusOf(double quantite, double seuil);
int statusRank(const Material &m);          // 0 rupture, 1 faible, 2 disponible
QString statusColor(const QString &status); // couleur du texte
QString statusTint(const QString &status);  // couleur de fond
double valueOf(const Material &m);          // quantité x prix unitaire

// --- unités
QStringList defaultUnits();
bool isIntUnit(const QString &unit);        // pièce, bobine... : quantités entières

// --- formatage (espaces comme séparateurs de milliers, virgule décimale)
QString fmtNum(double n);
QString fmtMoney(double n);                 // 3 décimales (dinar)
QString fmtDate(const QDate &d);
QString normalize(const QString &s);        // minuscules sans accents (recherche)
bool parseNum(const QString &text, double *out);   // accepte la virgule

// --- validation
struct RawMaterial {
    QString nom, categorie, fournisseur, quantite, unite, seuil, prix;
};
// Le nom est la clé primaire : il doit être unique (nameAlreadyUsed = un autre enregistrement porte déjà ce nom).
Errors validateMaterial(const RawMaterial &raw, bool nameAlreadyUsed,
                        const QStringList &categories, const QStringList &suppliers, Material *out);

// --- règles de saisie : un champ « nom » ne contient que des lettres (espaces, tirets et apostrophes entre les mots)
bool isValidName(const QString &text);
QString validateCategory(const QString &name, const QStringList &existing, const QString &ignore);
Errors validateSupplier(const Supplier &s, const QStringList &existingNames, const QString &ignoreName);

// --- estimation de consommation
Estimate estimate(const Material &m, double days, bool manual, double manualQty, const QString &per,
                  double historySum, int windowDays);

}  // namespace Stock
