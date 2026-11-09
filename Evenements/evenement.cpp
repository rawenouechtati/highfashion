#include "evenement.h"

#include <QPair>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
QString g_erreur;

void memoriser(const QSqlQuery &q) { g_erreur = q.lastError().text(); }

// Colonnes lues, toujours dans cet ordre (cf. depuisRequete)
const char *kColonnes =
    "ID, NOM, TYPE_EVENEMENT, DATE_EVENEMENT, HEURE, "
    "LIEU, DESCRIPTION, CAPACITE, STATUT, IMAGE_PATH";

// Libelle affiche (avec accents)  <->  valeur stockee en base (sans accents, cf. CHECK
// CK_EVENEMENT_TYPE et CK_EVENEMENT_STATUT du script v5)
using Correspondance = QList<QPair<QString, QString>>;

const Correspondance &tableTypes()
{
    static const Correspondance t = {
        { QStringLiteral("Défilé"), QStringLiteral("Defile") },
        { QStringLiteral("Atelier"), QStringLiteral("Atelier") },
        { QStringLiteral("Vente"), QStringLiteral("Vente") },
        { QStringLiteral("Salon"), QStringLiteral("Salon") },
        { QStringLiteral("Lancement"), QStringLiteral("Lancement") },
        { QStringLiteral("Autre"), QStringLiteral("Autre") }
    };
    return t;
}

const Correspondance &tableStatuts()
{
    static const Correspondance t = {
        { QStringLiteral("Planifié"), QStringLiteral("Planifie") },
        { QStringLiteral("Programmé"), QStringLiteral("Programme") },
        { QStringLiteral("Terminé"), QStringLiteral("Termine") },
        { QStringLiteral("Annulé"), QStringLiteral("Annule") }
    };
    return t;
}

QString versBase(const Correspondance &table, const QString &libelle)
{
    for (const auto &p : table)
        if (p.first == libelle)
            return p.second;
    return libelle;
}

QString depuisBase(const Correspondance &table, const QString &valeur)
{
    for (const auto &p : table)
        if (p.second == valeur)
            return p.first;
    return valeur;
}

QStringList libelles(const Correspondance &table)
{
    QStringList l;
    for (const auto &p : table)
        l << p.first;
    return l;
}
} // namespace

Evenement::Evenement(int id, const QString &nom, const QString &type, const QDate &date,
                     const QString &heure, const QString &lieu, const QString &description,
                     int capacite, const QString &statut, const QString &imageAffiche)
    : m_id(id), m_nom(nom), m_type(type), m_heure(heure), m_lieu(lieu),
      m_description(description), m_statut(statut), m_image(imageAffiche),
      m_date(date), m_capacite(capacite)
{
}

QStringList Evenement::typesAutorises()   { return libelles(tableTypes()); }
QStringList Evenement::statutsAutorises() { return libelles(tableStatuts()); }

QString Evenement::derniereErreur() { return g_erreur; }

bool Evenement::valider(QString *erreur) const
{
    auto ko = [&](const QString &msg) { if (erreur) *erreur = msg; return false; };

    // longueurs en OCTETS (colonnes VARCHAR2(n BYTE) : un caractere accentue compte pour 2)
    if (m_nom.trimmed().isEmpty())          return ko("Le nom de l'événement est obligatoire.");
    if (m_nom.toUtf8().size() > 150)        return ko("Le nom est trop long (150 caractères maximum).");
    if (!typesAutorises().contains(m_type)) return ko("Type d'événement non autorisé.");
    if (!statutsAutorises().contains(m_statut)) return ko("Statut non autorisé.");
    if (!m_date.isValid())                  return ko("La date est invalide.");
    static const QRegularExpression reHeure("^([01][0-9]|2[0-3]):[0-5][0-9]$");
    if (!reHeure.match(m_heure).hasMatch()) return ko("L'heure doit être au format HH:MM (ex. 18:30).");
    if (m_lieu.trimmed().isEmpty())         return ko("Le lieu est obligatoire.");
    if (m_lieu.toUtf8().size() > 150)       return ko("Le lieu est trop long (150 caractères maximum).");
    if (m_capacite <= 0 || m_capacite > 99999)
        return ko("La capacité doit être comprise entre 1 et 99999.");
    if (m_description.toUtf8().size() > 1000)
        return ko("La description est trop longue (1000 caractères maximum).");
    if (m_image.toUtf8().size() > 255)      return ko("Le chemin de l'image est trop long (255 caractères maximum).");
    return true;
}

Evenement Evenement::depuisRequete(QSqlQuery &q)
{
    return Evenement(q.value(0).toInt(),
                     q.value(1).toString(),
                     depuisBase(tableTypes(), q.value(2).toString()),
                     q.value(3).toDate(),
                     q.value(4).toString(),
                     q.value(5).toString(),
                     q.value(6).toString(),
                     q.value(7).toInt(),
                     depuisBase(tableStatuts(), q.value(8).toString()),
                     q.value(9).toString());
}

// Valeurs communes a l'INSERT et a l'UPDATE (memes noms de parametres)
void Evenement::lierValeurs(QSqlQuery &q) const
{
    q.bindValue(":nom", m_nom.trimmed());
    q.bindValue(":type", versBase(tableTypes(), m_type));
    q.bindValue(":statut", versBase(tableStatuts(), m_statut));
    q.bindValue(":date", m_date);
    q.bindValue(":heure", m_heure);
    q.bindValue(":lieu", m_lieu.trimmed());
    q.bindValue(":descr", m_description);
    q.bindValue(":capacite", m_capacite);
    q.bindValue(":image", m_image);
}

// ----------------------------------------------------------------------
//  CRUD
// ----------------------------------------------------------------------
bool Evenement::ajouter()
{
    g_erreur.clear();
    QString err;
    if (!valider(&err)) { g_erreur = err; return false; }

    // ID fourni par la sequence EVENEMENT_SEQ (meme methode que Employe::ajouter)
    QSqlQuery q;
    q.prepare("INSERT INTO EVENEMENT (ID, NOM, TYPE_EVENEMENT, STATUT, DATE_EVENEMENT, HEURE, "
              "LIEU, DESCRIPTION, CAPACITE, IMAGE_PATH) "
              "VALUES (EVENEMENT_SEQ.NEXTVAL, :nom, :type, :statut, :date, :heure, "
              ":lieu, :descr, :capacite, :image)");
    lierValeurs(q);
    if (!q.exec()) { memoriser(q); return false; }

    // recupere l'ID genere pour que *this reste coherent avec la base
    QSqlQuery requeteId;
    if (requeteId.exec("SELECT EVENEMENT_SEQ.CURRVAL FROM DUAL") && requeteId.next())
        m_id = requeteId.value(0).toInt();
    return true;
}

bool Evenement::modifier() const
{
    g_erreur.clear();
    QString err;
    if (!valider(&err)) { g_erreur = err; return false; }

    QSqlQuery q;
    q.prepare("UPDATE EVENEMENT SET NOM = :nom, TYPE_EVENEMENT = :type, STATUT = :statut, "
              "DATE_EVENEMENT = :date, HEURE = :heure, LIEU = :lieu, DESCRIPTION = :descr, "
              "CAPACITE = :capacite, IMAGE_PATH = :image "
              "WHERE ID = :id");
    lierValeurs(q);
    q.bindValue(":id", m_id);

    if (!q.exec()) { memoriser(q); return false; }
    if (q.numRowsAffected() <= 0) { g_erreur = "Événement introuvable (déjà supprimé ?)."; return false; }
    return true;
}

bool Evenement::supprimer(int id)
{
    g_erreur.clear();
    // Les lignes de PARTICIPER referencent l'evenement (pas de ON DELETE CASCADE dans le
    // script v5) : on les retire d'abord, dans la meme transaction.
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    QSqlQuery q;
    q.prepare("DELETE FROM PARTICIPER WHERE ID_EVENEMENT = :id");
    q.bindValue(":id", id);
    if (!q.exec()) { memoriser(q); db.rollback(); return false; }

    q.prepare("DELETE FROM EVENEMENT WHERE ID = :id");
    q.bindValue(":id", id);
    if (!q.exec()) { memoriser(q); db.rollback(); return false; }
    const bool supprime = q.numRowsAffected() > 0;
    if (!supprime) g_erreur = "Événement introuvable (déjà supprimé ?).";

    db.commit();
    return supprime;
}

QVector<Evenement> Evenement::afficher()
{
    return rechercher(QString(), Tri::DateAsc);
}

bool Evenement::trouver(int id, Evenement *resultat)
{
    g_erreur.clear();
    QSqlQuery q;
    q.prepare(QString("SELECT %1 FROM EVENEMENT WHERE ID = :id").arg(QString::fromLatin1(kColonnes)));
    q.bindValue(":id", id);
    if (!q.exec()) { memoriser(q); return false; }
    if (!q.next()) return false;
    if (resultat) *resultat = depuisRequete(q);
    return true;
}

QVector<Evenement> Evenement::rechercher(const QString &texte, Tri tri)
{
    g_erreur.clear();
    // ORDER BY ne peut pas etre lie par bindValue : valeurs choisies dans une liste fixe.
    QString ordre;
    switch (tri) {
    case Tri::DateAsc:  ordre = "DATE_EVENEMENT ASC, HEURE ASC"; break;
    case Tri::DateDesc: ordre = "DATE_EVENEMENT DESC, HEURE DESC"; break;
    case Tri::NomAsc:   ordre = "NOM ASC"; break;
    case Tri::LieuPlusFrequent:
        ordre = "(SELECT COUNT(*) FROM EVENEMENT e2 WHERE e2.LIEU = EVENEMENT.LIEU) DESC, LIEU ASC";
        break;
    }

    const QString t = texte.trimmed();
    QString sql = QString("SELECT %1 FROM EVENEMENT").arg(QString::fromLatin1(kColonnes));
    if (!t.isEmpty())
        sql += " WHERE UPPER(NOM) LIKE UPPER(:motif1)"
               " OR UPPER(LIEU) LIKE UPPER(:motif2)"
               " OR UPPER(TYPE_EVENEMENT) LIKE UPPER(:motif3)"
               " OR UPPER(STATUT) LIKE UPPER(:motif4)";
    sql += " ORDER BY " + ordre;

    QSqlQuery q;
    q.prepare(sql);
    if (!t.isEmpty()) {
        const QString motif = "%" + t + "%";
        q.bindValue(":motif1", motif);      // un parametre par occurrence : plus sur avec ODBC
        q.bindValue(":motif2", motif);
        q.bindValue(":motif3", motif);
        q.bindValue(":motif4", motif);
    }

    QVector<Evenement> liste;
    if (!q.exec()) { memoriser(q); return liste; }
    while (q.next())
        liste.push_back(depuisRequete(q));
    return liste;
}

// ----------------------------------------------------------------------
//  PARTICIPER
// ----------------------------------------------------------------------
bool Evenement::ajouterParticipant(int idEvenement, int idEmploye)
{
    g_erreur.clear();
    QSqlQuery q;
    q.prepare("INSERT INTO PARTICIPER (ID_EMPLOYE, ID_EVENEMENT) VALUES (:emp, :evt)");
    q.bindValue(":emp", idEmploye);
    q.bindValue(":evt", idEvenement);
    if (!q.exec()) { memoriser(q); return false; }
    return true;
}

bool Evenement::retirerParticipant(int idEvenement, int idEmploye)
{
    g_erreur.clear();
    QSqlQuery q;
    q.prepare("DELETE FROM PARTICIPER WHERE ID_EMPLOYE = :emp AND ID_EVENEMENT = :evt");
    q.bindValue(":emp", idEmploye);
    q.bindValue(":evt", idEvenement);
    if (!q.exec()) { memoriser(q); return false; }
    return q.numRowsAffected() > 0;
}

int Evenement::nombreParticipants(int idEvenement)
{
    g_erreur.clear();
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM PARTICIPER WHERE ID_EVENEMENT = :evt");
    q.bindValue(":evt", idEvenement);
    if (!q.exec() || !q.next()) { memoriser(q); return -1; }
    return q.value(0).toInt();
}
