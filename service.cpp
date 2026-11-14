#include "service.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QDebug>
#include <QVector>
#include <QStringList>
#include <QDate>

// ---------------------------------------------------------------------------
// Schéma Oracle (export1_corrige v5) :
//   SERVICE(ID, TYPE_SERVICE, DATE_DEBUT, DATE_FIN_PREVUE, TARIF, COUT_SERVICE,
//           ACOMPTE_VERSE, SOLDE_RESTANT, BENEFICE, STATUT, REFERENCE)
//   PASSER(ID_SERVICE, ID_CLIENT)  : lien Service-Client (géré ici)
//   ARTICLE.ID_SERVICE / EMPLOYE.ID_SERVICE : appartiennent aux modules Article / Employé,
//   donc la fiche ne les modifie pas.
// ID : généré par SERVICE_SEQ, jamais affiché.
// Référence (SRV-2026-0001) : colonne SERVICE.REFERENCE, UNIQUE (contrainte UQ_SERVICE_REFERENCE),
// générée à l'ajout à partir de l'ID. C'est la valeur unique visible dans le tableau.
// ---------------------------------------------------------------------------

// Référence lisible : SRV-<année de début>-<ID sur 4 chiffres> (unique car l'ID l'est)
static QString construireReference(int id, const QDate &debut)
{
    const int annee = debut.isValid() ? debut.year() : QDate::currentDate().year();
    return QString("SRV-%1-%2").arg(annee).arg(id, 4, 10, QChar('0'));
}

// La base écrit "Termine" (sans accent, contrainte CK_SERVICE_STATUT), l'interface "Terminé"
static QString statutVersDb(const QString &s)
{
    return s == QString::fromUtf8("Terminé") ? QString("Termine") : s;
}

static QString statutDepuisDb(const QString &s)
{
    return s == "Termine" ? QString::fromUtf8("Terminé") : s;
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

// Enregistre le client du service dans PASSER (remplace l'ancien lien)
static bool enregistrerClient(int idService, int idClient)
{
    QSqlQuery del;
    del.prepare("DELETE FROM passer WHERE id_service = :s");
    del.bindValue(":s", idService);
    if (!del.exec()) {
        qDebug() << "Erreur DELETE passer :" << del.lastError().text();
        return false;
    }
    if (idClient <= 0)
        return true;                       // client facultatif

    QSqlQuery ins;
    ins.prepare("INSERT INTO passer (id_service, id_client) VALUES (:s, :c)");
    ins.bindValue(":s", idService);
    ins.bindValue(":c", idClient);
    if (!ins.exec()) {
        qDebug() << "Erreur INSERT passer :" << ins.lastError().text();
        return false;
    }
    return true;
}

// 1. AJOUTER : ID pris dans SERVICE_SEQ, puis SERVICE + PASSER dans une même transaction
bool Service::ajouter()
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q;
    if (!q.exec("SELECT service_seq.NEXTVAL FROM dual") || !q.next()) {
        qDebug() << "Erreur SERVICE_SEQ :" << q.lastError().text();
        return false;
    }
    const int nouvelId = q.value(0).toInt();

    const QString nouvelleRef = construireReference(nouvelId, debut);

    db.transaction();
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
    query.bindValue(":statut", statutVersDb(statut));

    if (!query.exec()) {
        qDebug() << "Erreur INSERT :" << query.lastError().text();
        db.rollback();
        return false;
    }
    if (!enregistrerClient(nouvelId, idClient)) {
        db.rollback();
        return false;
    }
    if (!db.commit()) {
        qDebug() << "Erreur COMMIT :" << db.lastError().text();
        return false;
    }
    id = QString::number(nouvelId);
    reference = nouvelleRef;
    return true;
}

// 2. MODIFIER
bool Service::modifier()
{
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

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
    query.bindValue(":statut", statutVersDb(statut));
    query.bindValue(":id", id.toInt());

    if (!query.exec()) {
        qDebug() << "Erreur UPDATE :" << query.lastError().text();
        db.rollback();
        return false;
    }
    if (!enregistrerClient(id.toInt(), idClient)) {
        db.rollback();
        return false;
    }
    if (!db.commit()) {
        qDebug() << "Erreur COMMIT :" << db.lastError().text();
        return false;
    }
    return true;           // la référence ne change jamais
}

// 3. SUPPRIMER : retire d'abord les liens PASSER / UTILISER (tables d'association)
//    Oracle refuse (ORA-02292) si un article ou un employé référence encore ce service.
bool Service::supprimer(QString id)
{
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    for (const char *table : {"passer", "utiliser"}) {
        QSqlQuery d;
        d.prepare(QString("DELETE FROM %1 WHERE id_service = :id").arg(table));
        d.bindValue(":id", id.toInt());
        if (!d.exec()) {
            qDebug() << "Erreur DELETE" << table << ":" << d.lastError().text();
            db.rollback();
            return false;
        }
    }

    QSqlQuery query;
    query.prepare("DELETE FROM service WHERE id = :id");
    query.bindValue(":id", id.toInt());
    if (!query.exec()) {
        qDebug() << "Erreur DELETE :" << query.lastError().text();
        db.rollback();
        return false;
    }
    return db.commit();
}

// 4. AFFICHER (QSqlQueryModel) : référence affichée, ID non affiché
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
    if (!q.exec("SELECT s.id, s.type_service, s.date_debut, s.date_fin_prevue, s.tarif, "
                "s.cout_service, s.acompte_verse, s.statut, s.reference, "
                "(SELECT MIN(p.id_client) FROM passer p WHERE p.id_service = s.id) "
                "FROM service s ORDER BY s.id")) {
        qDebug() << "Erreur chargerDepuisDb :" << q.lastError().text();
        return liste;
    }
    while (q.next()) {
        Service s;
        const int idNum = q.value(0).toInt();
        s.id        = QString::number(idNum);
        s.type      = q.value(1).toString();
        s.debut     = q.value(2).toDate();
        s.fin       = q.value(3).toDate();
        s.tarif     = q.value(4).toDouble();
        s.cout      = q.value(5).toDouble();
        s.acompte   = q.value(6).toDouble();
        s.statut    = statutDepuisDb(q.value(7).toString());
        s.reference = q.value(8).toString();
        if (s.reference.isEmpty())                 // ancienne ligne sans référence : on la calcule
            s.reference = construireReference(idNum, s.debut);
        s.idClient  = q.value(9).toInt();          // NULL -> 0
        liste.append(s);
    }
    return liste;
}

// 6. VALIDER (contrôle de saisie)
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
        "En cours", "En attente", QString::fromUtf8("Terminé"), "En retard"
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

ListeChoix Service::listerClients()
{
    ListeChoix l;
    QSqlQuery q;
    if (!q.exec("SELECT id, TRIM(prenom || ' ' || nom) FROM client ORDER BY nom, prenom")) {
        qDebug() << "Erreur listerClients :" << q.lastError().text();
        return l;
    }
    while (q.next())
        l.append({q.value(0).toInt(), q.value(1).toString().trimmed()});
    return l;
}

QStringList Service::listerEmployes()
{
    return lireListe("SELECT TRIM(prenom || ' ' || nom) FROM employe ORDER BY nom, prenom");
}