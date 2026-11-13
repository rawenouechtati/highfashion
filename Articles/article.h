// article.h — entité « Article » (table Oracle ARTICLE, modèle physique v5).
//
// TOUTES les requêtes SQL du module Articles sont dans cette classe (architecture modèle-vue du prof) :
// la page (ModuleArticles) ne fait qu'appeler ajouter() / afficher() / modifier() / supprimer() et les
// méthodes de lecture ci-dessous. La connexion Oracle unique est ouverte dans main.cpp.
#pragma once
#include <QDate>
#include <QList>
#include <QMap>
#include <QSqlQueryModel>
#include <QString>
#include <QStringList>
#include <QVector>

class Article
{
public:
    // Saisie brute venant du formulaire (texte tel que tapé) : validée par verifier() avant tout accès SQL.
    struct Saisie {
        QString nom, maison, decennie, etat, prix, categorie, origine, statut;
        int idService = 0;
        QDate date;
    };

    Article() = default;
    Article(int id, const QString &nom, const QString &maison, int decennie, const QString &etat, double prix,
            const QString &categorie, const QString &origine, const QString &statut, int idService, const QDate &date);

    int getId() const { return id; }
    QString getNom() const { return nom; }
    QString getMaison() const { return maison; }
    int getDecennie() const { return decennie; }
    QString getEtat() const { return etat; }
    double getPrix() const { return prix; }
    QString getCategorie() const { return categorie; }
    QString getOrigine() const { return origine; }
    QString getStatut() const { return statut; }
    int getIdService() const { return idService; }
    QDate getDate() const { return date; }
    void setId(int v) { id = v; }
    void setPrix(double v) { prix = v; }
    QString erreur() const { return erreur_; }

    // --- CRUD (SQL préparé, bindValue) ---
    bool ajouter();                 // INSERT ... ARTICLE_SEQ.NEXTVAL ; renseigne l'ID généré
    QSqlQueryModel *afficher();     // SELECT pour le tableau (l'appelant détruit le modèle)
    bool modifier();                // UPDATE ... WHERE ID = :id
    bool supprimer(int idArticle);  // DELETE ... WHERE ID = :id

    // --- lectures complémentaires ---
    static QVector<Article> tous(QString *erreur = nullptr);     // lit via afficher()
    static bool tableAccessible(QString *erreur = nullptr);
    static bool existe(const QString &nom, const QString &maison, int ignorerId = 0);
    static QList<int> services(QString *erreur = nullptr);       // ID des services (clé étrangère ID_SERVICE)
    static QString messageErreur(const QString &texteOracle);    // message lisible pour une erreur ORA-xxxxx

    // --- contrôle de saisie (aucun SQL) : champ -> message ; vide = saisie valide ; remplit *out ---
    static QMap<QString, QString> verifier(const Saisie &s, Article *out = nullptr);
    static QString verifierChamp(const QString &champ, const Saisie &s);   // un seul champ (frappe en direct)

    // --- listes proposées (valeurs acceptées par les contraintes CHECK de la table) ---
    static QStringList etats();
    static QStringList statuts();
    static QStringList categories();
    static bool categoriesFermees();                               // true si la base impose une liste de catégories
    static QStringList valeursAutorisees(const QString &colonne);  // lue dans USER_CONSTRAINTS ("" = aucune contrainte IN)
    static void relireContraintes();
    static void analyserCondition(const QString &condition, QMap<QString, QStringList> &out);   // « COL IN ('a','b') »
    static QStringList maisons();
    static QList<int> decennies();

    // --- métier innovant : estimation du prix (aucun SQL) ---
    struct Estimation { double total = 0; double base = 0, maison = 0, decennie = 0, etat = 0; };
    static Estimation estimerPrix(const QString &maison, int decennie, const QString &etat);

private:
    int id = 0;
    QString nom, maison;
    int decennie = 0;
    QString etat;
    double prix = 0;
    QString categorie, origine, statut;
    int idService = 0;
    QDate date;
    QString erreur_;
};
