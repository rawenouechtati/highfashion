#ifndef SERVICE_H
#define SERVICE_H

#include <QString>
#include <QDate>
#include <QSqlQueryModel>
#include <QVector>
#include <QStringList>
#include <QPair>

typedef QVector<QPair<int, QString>> ListeChoix;

class Service
{
public:
    // Attributs (données)
    QString id, reference, article, client, type, employe, statut;
    int idClient = 0;      // lien Service-Client (table PASSER), 0 = aucun
    QDate debut, fin;
    double tarif, cout, acompte;

    // Constructeurs
    Service();
    Service(QString id, QString article, QString client, QString type, QString employe,
            QDate debut, QDate fin, double tarif, double cout, double acompte, QString statut);

    // CRUD (Fonctionnalités de base)
    bool ajouter();
    bool modifier();
    bool supprimer(QString id);

    // Afficher (retourne un modèle pour la vue)
    QSqlQueryModel* afficher();
    static QVector<Service> chargerDepuisDb();

    // Contrôle de saisie (validation avant enregistrement)
    // Retourne true si le service est valide.
    // En cas d'erreur, retourne false et remplit "erreur" avec un message clair.
    bool valider(QString &erreur) const;

    static bool existe(const QString &id);

    // Listes pour les ComboBox de la fiche (lues dans Oracle)
    static QStringList listerArticles();
    static ListeChoix listerClients();   // (ID Oracle, "Prénom Nom")
    static QStringList listerEmployes();
};

#endif // SERVICE_H