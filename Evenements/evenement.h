#ifndef EVENEMENT_H
#define EVENEMENT_H

#include <QDate>
#include <QString>
#include <QStringList>
#include <QVector>

// Entite Evenement (module Evenements, Melek) : toutes les requetes SQL du module sont ici.
//
// Table EVENEMENT (base Oracle v5, script export1_corrige_v5_FINAL.sql) :
//   ID, NOM, TYPE_EVENEMENT, STATUT, DATE_EVENEMENT, HEURE (HH:MM), LIEU, DESCRIPTION,
//   CAPACITE, IMAGE_PATH            (cle primaire ID, alimentee par la sequence EVENEMENT_SEQ)
// Table PARTICIPER (association Employe - Evenement) : ID_EMPLOYE, ID_EVENEMENT
//
// Valeurs de type et de statut : l'interface affiche des libelles avec accents
// (ex. "Défilé", "Planifié"), la base stocke des valeurs sans accents imposees par les
// contraintes CHECK (ex. 'Defile', 'Planifie'). La conversion est faite ici, dans la classe
// entite : l'interface n'a jamais besoin de la connaitre.
//
// Toutes les requetes sont preparees (prepare + bindValue). La connexion Oracle est celle
// ouverte une seule fois par main.cpp (classe Connection, Singleton) : QSqlQuery l'utilise
// par defaut.
class Evenement
{
public:
    Evenement() = default;
    Evenement(int id, const QString &nom, const QString &type, const QDate &date,
              const QString &heure, const QString &lieu, const QString &description,
              int capacite, const QString &statut, const QString &imageAffiche);

    // accesseurs
    int     id() const            { return m_id; }
    QString nom() const           { return m_nom; }
    QString type() const          { return m_type; }
    QDate   date() const          { return m_date; }
    QString heure() const         { return m_heure; }       // "HH:mm"
    QString lieu() const          { return m_lieu; }
    QString description() const   { return m_description; }
    int     capacite() const      { return m_capacite; }
    QString statut() const        { return m_statut; }
    QString imageAffiche() const  { return m_image; }

    void setId(int v)                      { m_id = v; }
    void setNom(const QString &v)          { m_nom = v; }
    void setType(const QString &v)         { m_type = v; }
    void setDate(const QDate &v)           { m_date = v; }
    void setHeure(const QString &v)        { m_heure = v; }
    void setLieu(const QString &v)         { m_lieu = v; }
    void setDescription(const QString &v)  { m_description = v; }
    void setCapacite(int v)                { m_capacite = v; }
    void setStatut(const QString &v)       { m_statut = v; }
    void setImageAffiche(const QString &v) { m_image = v; }

    // controle de saisie (memes regles que les contraintes de la table) ;
    // false + message lisible si invalide
    bool valider(QString *erreur = nullptr) const;

    // CRUD
    bool ajouter();                                   // renseigne id() apres insertion
    bool modifier() const;
    static bool supprimer(int id);
    static QVector<Evenement> afficher();
    static bool trouver(int id, Evenement *resultat);

    // recherche / tri cote SQL
    enum class Tri { DateAsc, DateDesc, LieuPlusFrequent, NomAsc };
    static QVector<Evenement> rechercher(const QString &texte, Tri tri = Tri::DateAsc);

    // table PARTICIPER
    static bool ajouterParticipant(int idEvenement, int idEmploye);
    static bool retirerParticipant(int idEvenement, int idEmploye);
    static int  nombreParticipants(int idEvenement);

    // libelles proposes dans l'interface (alignes sur les CHECK CK_EVENEMENT_TYPE / _STATUT)
    static QStringList typesAutorises();
    static QStringList statutsAutorises();

    // dernier message d'erreur (validation ou base de donnees) ; vide si la derniere operation a reussi
    static QString derniereErreur();

private:
    static Evenement depuisRequete(class QSqlQuery &q);
    void lierValeurs(class QSqlQuery &q) const;       // bindValue communs a ajouter() et modifier()

    int     m_id = 0;
    QString m_nom, m_type, m_heure, m_lieu, m_description, m_statut, m_image;
    QDate   m_date;
    int     m_capacite = 0;
};

#endif // EVENEMENT_H
