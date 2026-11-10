// stockdb.h — couche « service » du module Matières premières.
//
// AUCUNE requête SQL ici : les données viennent de l'entité Matiere (table Oracle MATIERE_PREMIERE).
// Cette classe garde en mémoire la liste des matières (rechargée après chaque modification),
// calcule les alertes de stock et les statistiques, et tient l'historique des mouvements de stock.
// Le schéma v5 ne contient pas de table de mouvements : l'historique est conservé dans un petit
// fichier JSON local (dossier de données de l'application) — la quantité en stock, elle, est
// bien enregistrée dans Oracle.
#pragma once
#include "matmodels.h"
#include "stockrules.h"
#include <QHash>
#include <QObject>
#include <QStringList>
#include <QVector>

namespace Mat {

class StockDb : public QObject
{
    Q_OBJECT
public:
    explicit StockDb(QObject *parent = nullptr);

    // Premier chargement : vérifie que la table existe, lit les matières. false = message dans lastError().
    bool charger();
    QString lastError() const { return lastError_; }
    void setHistoryFile(const QString &path);   // fichier JSON de l'historique ("" = pas de fichier)

    // --- matières premières (lues dans le cache ; recharger() relit Oracle)
    QVector<Material> materials() const { return materials_; }
    bool getMaterial(const QString &nom, Material *out) const;
    bool materialExists(const QString &nom) const { return getMaterial(nom, nullptr); }
    QStringList categories() const;      // catégories proposées : exemples + valeurs déjà utilisées
    QStringList supplierNames() const;   // fournisseurs déjà utilisés
    void recharger();                    // relit la table Oracle puis notifie les pages
    void notifyChanged() { recharger(); }   // à appeler après ajouter()/modifier()/supprimer()
    void materialRenamed(const QString &oldName, const QString &newName);   // l'historique suit le renommage

    // --- mouvements de stock
    QVector<Movement> movements() const;
    // Valide puis enregistre une entrée/sortie et met à jour la quantité en stock (Oracle).
    // Retourne les erreurs par champ (vide = succès).
    Stock::Errors recordMovement(const QString &materialId, const QString &type, const QString &quantityText,
                                 const QDate &date, const QString &description, Movement *out = nullptr);
    double consumedSum(const QString &materialId, int windowDays) const;   // somme des sorties

    // --- alertes (une par matière en alerte, sans doublon)
    QVector<Alert> alerts() const;
    int alertCount() const { return alerts_.size(); }
    void refreshAlerts() { recharger(); }

    // --- statistiques
    Stats stats() const;

signals:
    void changed();                                    // toute modification des données
    void alertsCreated(const QVector<Alert> &alerts);  // nouvelles alertes uniquement

private:
    QVector<Alert> syncAlerts();
    void loadHistory();
    void saveHistory() const;

    QVector<Material> materials_;
    QVector<Movement> history_;
    QHash<QString, Alert> alerts_;      // clé = nom en minuscules
    QString historyFile_;
    QString lastError_;
    int nextMovementId_ = 1;
};

}  // namespace Mat
