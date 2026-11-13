#include "service.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QVector>
#include <QStringList>
#include <QDate>

// ---------------------------------------------------------------------------
// SERVICE.ID (NUMBER) est généré automatiquement et n'est jamais affiché ;
// l'utilisateur voit la REFERENCE (SRV-2026-0001), unique grâce à une contrainte Oracle.
// Dans l'objet Service, "id" contient simplement ce nombre sous forme de texte
// ("12"), ou reste vide tant que le service n'est pas encore enregistré.
// Article / client / employé ne sont PAS dans la base : ils restent côté interface.
// ---------------------------------------------------------------------------

// Référence lisible et unique : SRV-<année>-<ID sur 4 chiffres> (ex : SRV-2026-0001)
static QString construireReference(int id)
{
    return QString("SRV-%1-%2").arg(QDate::currentDate().year()).arg(id, 4, 10, QChar('0'));
}

// Constructeur par défaut
Service::Service()
    : tarif(0), cout(0), acompte(0)
{}

// Constructeur avec paramètres
Service::Service(QString id, QString article, QString client, QString type, QString employe,
                 QDate debut, QDate fin, double tarif, double cout, double acompte, QString statut)
    : id(id), article(article), client(client), type(type), employe(employe), statut(statut),
    debut(debut), fin(fin), tarif(tarif), cout(cout), acompte(acompte)
{}

// 1. AJOUTER : l'ID est calculé ici (MAX + 1) puis rangé dans this->id
bool Service::ajouter()
{
    QSqlQuery q;
    if (!q.exec("SELECT NVL(MAX(id), 0) + 1 FROM service") || !q.next()) {
        qDebug() << "Erreur prochain ID :" << q.lastError().text();
        return false;
    }
    const int nouvelId = q.value(0).toInt();

    const QString nouvelleRef = construireReference(nouvelId);

    QSqlQuery query;
    query.prepare("INSERT INTO service (id, reference, type_service, date_debut, date_fin_prevue, "
                  "tarif, cout_service, acompte_verse, solde_restant, benefice, statut) "
                  "VALUES (:id, :ref, :type, :debut, :fin, :tarif, :cout, :acompte, :solde, :benef, :statut)");
    query.bindValue(":id", nouvelId);
    query.bindValue(":ref", nouvelleRef);
    query.bindValue(":type", type);
    query.bindValue(":debut", debut);
    query.bindValue(":fin", fin);
    query.bindValue(":tarif", tarif);
    query.bindValue(":cout", cout);
    query.bindValue(":acompte", acompte);
    query.bindValue(":solde", tarif - acompte);
    query.bindValue(":benef", tarif - cout);
    query.bindValue(":statut", statut);

    if (!query.exec()) {
        qDebug() << "Erreur INSERT :" << query.lastError().text();
        return false;
    }
    id = QString::number(nouvelId);
    reference = nouvelleRef;
    return true;
}

// 2. MODIFIER
bool Service::modifier()
{
    QSqlQuery query;
    query.prepare("UPDATE service SET type_service=:type, date_debut=:debut, date_fin_prevue=:fin, "
                  "tarif=:tarif, cout_service=:cout, acompte_verse=:acompte, "
                  "solde_restant=:solde, benefice=:benef, statut=:statut WHERE id=:id");
    query.bindValue(":type", type);
    query.bindValue(":debut", debut);
    query.bindValue(":fin", fin);
    query.bindValue(":tarif", tarif);
    query.bindValue(":cout", cout);
    query.bindValue(":acompte", acompte);
    query.bindValue(":solde", tarif - acompte);
    query.bindValue(":benef", tarif - cout);
    query.bindValue(":statut", statut);
    query.bindValue(":id", id.toInt());

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
    query.bindValue(":id", id.toInt());

    if (!query.exec()) {
        qDebug() << "Erreur DELETE :" << query.lastError().text();
        return false;
    }
    return true;
}

// 4. AFFICHER (QSqlQueryModel) : l'ID n'est pas sélectionné
QSqlQueryModel* Service::afficher()
{
    QSqlQueryModel* model = new QSqlQueryModel();
    model->setQuery("SELECT reference, type_service, date_debut, date_fin_prevue, tarif, "
                    "cout_service, acompte_verse, statut FROM service ORDER BY id");
    if (model->lastError().isValid())
        qDebug() << "Erreur afficher :" << model->lastError().text();

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("Référence"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Type"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Début"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Fin"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Tarif"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Coût"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Acompte"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Statut"));
    return model;
}

// 5. CHARGER POUR LE QTableWidget
QVector<Service> Service::chargerDepuisDb()
{
    QVector<Service> liste;
    QSqlQuery q;
    if (!q.exec("SELECT id, reference, type_service, date_debut, date_fin_prevue, "
                "tarif, cout_service, acompte_verse, statut "
                "FROM service ORDER BY id")) {
        qDebug() << "Erreur chargerDepuisDb :" << q.lastError().text();
        return liste;
    }
    while (q.next()) {
        Service s;
        s.id      = q.value(0).toString();
        s.reference = q.value(1).toString();
        s.type    = q.value(2).toString();
        s.debut   = q.value(3).toDate();
        s.fin     = q.value(4).toDate();
        s.tarif   = q.value(5).toDouble();
        s.cout    = q.value(6).toDouble();
        s.acompte = q.value(7).toDouble();
        s.statut  = q.value(8).toString();
        liste.append(s);
    }
    return liste;
}

// 6. VALIDER (contrôle de saisie) : plus de contrôle sur l'ID ni sur l'article
bool Service::valider(QString &erreur) const
{
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

// 7. EXISTE
bool Service::existe(const QString &id)
{
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM service WHERE id = :id");
    q.bindValue(":id", id.toInt());
    if (!q.exec()) {
        qDebug() << "Erreur existe :" << q.lastError().text();
        return false;
    }
    return q.next() && q.value(0).toInt() > 0;
}

// 8. LISTES POUR LES COMBOBOX (la vue ne contient aucune requête SQL)
static QStringList lireListe(const QString &sql)
{
    QStringList l;
    QSqlQuery q;
    if (!q.exec(sql)) {
        qDebug() << "Erreur liste :" << sql << q.lastError().text();
        return l;
    }
    while (q.next()) {
        const QString v = q.value(0).toString().trimmed();
        if (!v.isEmpty())
            l << v;
    }
    return l;
}

QStringList Service::listerArticles()
{
    return lireListe("SELECT nom FROM article ORDER BY nom");
}

QStringList Service::listerClients()
{
    return lireListe("SELECT TRIM(prenom || ' ' || nom) FROM client ORDER BY nom, prenom");
}

QStringList Service::listerEmployes()
{
    return lireListe("SELECT TRIM(prenom || ' ' || nom) FROM employe ORDER BY nom, prenom");
}