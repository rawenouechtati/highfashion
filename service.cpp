#include "service.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QVector>
#include <QStringList>
#include <QMetaType>

// Constructeur par défaut
Service::Service()
    : tarif(0), cout(0), acompte(0)
{}

// Constructeur avec paramètres
Service::Service(QString id, QString article, QString client, QString type, QString employe,
                 QDate debut, QDate fin, double tarif, double cout, double acompte, QString statut)
    : id(id), article(article), client(client), type(type), employe(employe),
    debut(debut), fin(fin), tarif(tarif), cout(cout), acompte(acompte), statut(statut)
{}

// 1. AJOUTER
bool Service::ajouter()
{
    QSqlQuery query;
    query.prepare("INSERT INTO service (id, article, client, type_service, employe, "
                  "date_debut, date_fin, tarif, cout, acompte, statut) "
                  "VALUES (:id, :article, :client, :type, :employe, "
                  ":debut, :fin, :tarif, :cout, :acompte, :statut)");

    query.bindValue(":id", id);
    query.bindValue(":article", article);
    query.bindValue(":client", client.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : QVariant(client));
    query.bindValue(":type", type);
    query.bindValue(":employe", employe);
    query.bindValue(":debut", debut);
    query.bindValue(":fin", fin);
    query.bindValue(":tarif", tarif);
    query.bindValue(":cout", cout);
    query.bindValue(":acompte", acompte);
    query.bindValue(":statut", statut);

    if (!query.exec()) {
        qDebug() << "Erreur INSERT :" << query.lastError().text();
        return false;
    }
    return true;
}

// 2. MODIFIER
bool Service::modifier()
{
    QSqlQuery query;
    query.prepare("UPDATE service SET article=:article, client=:client, type_service=:type, "
                  "employe=:employe, date_debut=:debut, date_fin=:fin, tarif=:tarif, "
                  "cout=:cout, acompte=:acompte, statut=:statut WHERE id=:id");

    query.bindValue(":article", article);
    query.bindValue(":client", client.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : QVariant(client));
    query.bindValue(":type", type);
    query.bindValue(":employe", employe);
    query.bindValue(":debut", debut);
    query.bindValue(":fin", fin);
    query.bindValue(":tarif", tarif);
    query.bindValue(":cout", cout);
    query.bindValue(":acompte", acompte);
    query.bindValue(":statut", statut);
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Erreur UPDATE :" << query.lastError().text();
        return false;
    }
    return true;
}

// 3. SUPPRIMER
bool Service::supprimer(QString id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM service WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Erreur DELETE :" << query.lastError().text();
        return false;
    }
    return true;
}

// 4. AFFICHER (Pour le professeur, avec QSqlQueryModel)
QSqlQueryModel* Service::afficher()
{
    QSqlQueryModel* model = new QSqlQueryModel();
    model->setQuery("SELECT id, article, client, type_service, employe, date_debut, date_fin, tarif, cout, acompte, statut FROM service ORDER BY id");
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Article"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Client"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Type"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Employé"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Début"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Fin"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Tarif"));
    return model;
}

// 5. CHARGER POUR LE QTableWidget (Pour garder ton beau design)
QVector<Service> Service::chargerDepuisDb()
{
    QVector<Service> liste;
    QSqlQuery q("select id, article, client, type_service, employe, "
                "date_debut, date_fin, tarif, cout, acompte, statut "
                "from service order by id");
    while (q.next()) {
        Service s;
        s.id = q.value(0).toString();   s.article = q.value(1).toString();
        s.client = q.value(2).toString(); s.type = q.value(3).toString();
        s.employe = q.value(4).toString(); s.debut = q.value(5).toDate();
        s.fin = q.value(6).toDate();     s.tarif = q.value(7).toDouble();
        s.cout = q.value(8).toDouble();  s.acompte = q.value(9).toDouble();
        s.statut = q.value(10).toString();
        liste.append(s);
    }
    return liste;
}

// 6. VALIDER (contrôle de saisie)
bool Service::valider(QString &erreur) const
{
    if (id.trimmed().isEmpty()) {
        erreur = "L'identifiant du service est obligatoire.";
        return false;
    }
    if (!id.startsWith("SV") || id.length() < 3) {
        erreur = "L'identifiant doit commencer par 'SV' (ex: SV001).";
        return false;
    }
    if (article.trimmed().length() < 3) {
        erreur = "L'article doit contenir au moins 3 caractères.";
        return false;
    }
    if (!debut.isValid() || !fin.isValid()) {
        erreur = "Les dates de début et de fin doivent être renseignées.";
        return false;
    }
    if (fin < debut) {
        erreur = "La date de fin doit être postérieure ou égale à la date de début.";
        return false;
    }
    if (tarif <= 0) {
        erreur = "Le tarif doit être strictement supérieur à 0.";
        return false;
    }
    if (cout < 0) {
        erreur = "Le coût ne peut pas être négatif.";
        return false;
    }
    if (acompte < 0) {
        erreur = "L'acompte ne peut pas être négatif.";
        return false;
    }
    if (acompte > tarif) {
        erreur = "L'acompte ne peut pas dépasser le tarif.";
        return false;
    }
    const QStringList typesValides = {
        "Location", "Restauration", "Nettoyage", "Authentification"
    };
    if (!typesValides.contains(type)) {
        erreur = "Le type doit être : Location, Restauration, Nettoyage ou Authentification.";
        return false;
    }
    const QStringList statutsValides = {
        "En cours", "En attente", "Terminé", "En retard"
    };
    if (!statutsValides.contains(statut)) {
        erreur = "Le statut doit être : En cours, En attente, Terminé ou En retard.";
        return false;
    }
    erreur.clear();
    return true;
}