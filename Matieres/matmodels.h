// matmodels.h — structures de données simples (pas de logique ici)
#pragma once
#include <QDate>
#include <QMap>
#include <QString>
#include <QVector>

namespace Mat {


struct Material {
    int id = 0;                                   // ID (clé primaire Oracle)
    QString nom, categorie, fournisseur, unite;   // le nom est unique (contrôlé dans l'application)
    double quantite = 0, seuil = 0, prix = 0;
};

struct Movement {
    int id = 0;
    QDate date;
    QString materialId, nom, unite, type, description;   // materialId = nom de la matière ; type : "Entrée" ou "Sortie"
    double quantite = 0;
};

struct Alert {
    QString materialId, nom, statut, unite, signature;
    double quantite = 0, seuil = 0;
    QDate date;
};

struct CategoryStat {
    QString categorie;
    int refs = 0;
    double value = 0;
    QMap<QString, double> units;   // quantité par unité dans cette catégorie
};

struct Stats {
    int refs = 0, low = 0, out = 0;
    double totalValue = 0;
    QMap<QString, double> byUnit;
    QVector<CategoryStat> byCategory;
    QVector<Material> replenish;   // matières en alerte (faible ou rupture)
};

struct Estimate {
    bool ok = false, noHistory = false, manual = false, sufficient = false;
    QString error;
    Material material;
    int windowDays = 0;
    double sum = 0, daily = 0, days = 0, need = 0, stock = 0, diff = 0, autonomy = 0;
};

}  // namespace Mat
