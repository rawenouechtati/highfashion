// Database.h — couche d'accès aux données (SQLite via QtSql)
// Toute la persistance passe par cette classe : pour utiliser un autre moteur
// (MySQL, PostgreSQL...), il suffit de modifier Database.cpp.
#pragma once
#include "Models.h"
#include "Stock.h"
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QVariantList>

class Database : public QObject
{
    Q_OBJECT
public:
    explicit Database(QObject *parent = nullptr);
    ~Database() override;

    // Ouvre la base.
    //   "odbc:NOM_DSN" (ou "odbc") -> Oracle via ODBC (QODBC) : les tables doivent exister
    //                                 (exécuter gestion_materiel_oracle.sql une fois) ;
    //   ":memory:" ou fichier .sqlite -> SQLite (tests / base temporaire de secours).
    bool open(const QString &target, QString *error = nullptr);
    static QString defaultTarget();       // cible Oracle par défaut (DSN ODBC)
    QString lastError() const { return lastError_; }   // dernier message d'erreur SQL (vide si succès)

    // Connexion unique (patron Singleton) réutilisée par les classes entités (ex. Fournisseur)
    QSqlDatabase connection() const { return db_; }
    QString orderColumn() const { return oracle_ ? QStringLiteral("ordre") : QStringLiteral("rowid"); }
    void reportError(const QSqlQuery &q) const { noteError(q, q.lastQuery()); }
    void notifyChanged() { finishChange(); }          // à appeler après une modification des données

    // --- matières premières
    QVector<Material> materials() const;
    bool getMaterial(const QString &id, Material *out) const;
    bool materialExists(const QString &id) const;
    // Ajout / modification / suppression / identifiant suivant : voir la classe Matiere (Matiere.h)

    // --- catégories
    QStringList categories() const;
    int categoryUsage(const QString &name) const;
    bool addCategory(const QString &name);
    bool renameCategory(const QString &oldName, const QString &newName);
    bool deleteCategory(const QString &name);   // refuse si la catégorie est utilisée

    // --- fournisseurs
    QVector<Supplier> suppliers() const;
    QStringList supplierNames() const;
    int supplierUsage(const QString &name) const;
    // Création / modification / suppression : voir la classe Fournisseur (Fournisseur.h)

    // --- mouvements de stock
    QVector<Movement> movements() const;
    // Valide puis enregistre une entrée/sortie et met à jour la quantité en stock.
    // Retourne les erreurs par champ (vide = succès).
    Stock::Errors recordMovement(const QString &materialId, const QString &type, const QString &quantityText,
                                 const QDate &date, const QString &description, Movement *out = nullptr);
    double consumedSum(const QString &materialId, int windowDays) const;   // somme des sorties

    // --- alertes (une par matière en alerte, sans doublon)
    QVector<Alert> alerts() const;
    int alertCount() const;
    void refreshAlerts();

    // --- statistiques
    Stats stats() const;

    // --- jeu de données
    bool isDemo() const;
    void loadDemo();    // remplace tout par des données de DÉMONSTRATION
    void clearAll();    // efface tout (garde les catégories d'exemple)

signals:
    void changed();                                  // toute modification des données
    void alertsCreated(const QVector<Alert> &alerts);  // nouvelles alertes uniquement

private:
    bool exec(const QString &sql, const QVariantList &args = {}) const;   // requête préparée + paramètres liés
    bool run(QSqlQuery &q, const QString &sql = QString()) const;        // exécute (et journalise l'erreur éventuelle)
    void noteError(const QSqlQuery &q, const QString &sql) const;
    bool setMeta(const QString &key, const QString &value);
    bool createTables();
    QVector<Alert> syncAlerts();
    void finishChange();   // synchronise les alertes puis émet changed()
    void insertDefaultCategories();

    QSqlDatabase db_;
    QString conn_;
    bool oracle_ = false;               // true : Oracle via ODBC ; false : SQLite
    mutable QString lastError_;
};
