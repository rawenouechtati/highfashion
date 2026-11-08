#ifndef EMPLOYE_H
#define EMPLOYE_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>

class QSqlQueryModel;

// Modele de donnees pour un employe (module Gestion des employes).
// Classe simple, sans dependance a la base de donnees : adaptee plus tard
// pour Oracle (getters/setters deja prets pour ca).
class Employe
{
public:
    Employe();
    Employe(int id, const QString &nom, const QString &prenom, const QString &poste,
            const QString &email, const QString &telephone,
            const QString &statut, const QString &motDePasse,
            const QString &posteTravail = QString(), int idService = -1);

    int id() const;
    void setId(int id);

    QString nom() const;
    void setNom(const QString &nom);

    QString prenom() const;
    void setPrenom(const QString &prenom);

    QString poste() const;
    void setPoste(const QString &poste);

    QString email() const;
    void setEmail(const QString &email);

    QString telephone() const;
    void setTelephone(const QString &telephone);

    QString statut() const;
    void setStatut(const QString &statut);

    QString motDePasse() const;
    void setMotDePasse(const QString &motDePasse);

    // Poste de travail ou l'employe est actuellement affecte : texte de la colonne
    // EMPLOYE.POSTE_TRAVAIL (voir PosteTravail::tous()), chaine vide = aucun poste.
    QString posteTravail() const;
    void setPosteTravail(const QString &posteTravail);

    // Service de l'employe : colonne EMPLOYE.ID_SERVICE, obligatoire (cle etrangere vers
    // SERVICE.ID). -1 = pas encore choisi.
    int idService() const;
    void setIdService(int idService);

    // --- Regles de validation (memes que les contraintes de la table EMPLOYE). Chacune renvoie
    // true si la valeur est acceptable, sinon false et un message precis dans *erreur. La Vue
    // les appelle a chaque frappe ; elle ne contient aucune regle elle-meme. ---
    static bool nomValide(const QString &nom, QString *erreur = nullptr);
    static bool prenomValide(const QString &prenom, QString *erreur = nullptr);
    static bool telephoneValide(const QString &telephone, QString *erreur = nullptr);
    static bool emailValide(const QString &email, QString *erreur = nullptr);
    // obligatoire = false : un mot de passe vide est accepte (modification : on garde l'ancien).
    static bool motDePasseValide(const QString &motDePasse, QString *erreur = nullptr, bool obligatoire = true);
    static bool posteValide(const QString &poste, QString *erreur = nullptr);       // role
    static bool statutValide(const QString &statut, QString *erreur = nullptr);
    static bool posteTravailValide(const QString &poste, QString *erreur = nullptr); // "" = aucun
    // idService : identifiant sous forme de texte (donnee de la liste deroulante) ; obligatoire.
    static bool serviceValide(const QString &idService, QString *erreur = nullptr);
    // Valeurs autorisees par les contraintes CHECK de la base.
    static QStringList postes();
    static QStringList statuts();

    // --- Acces base de donnees (toutes les requetes SQL du module Employes vivent ici,
    // jamais dans GEmployes/Connexion -- cf. consigne "Aucune requete SQL... en dehors
    // de la classe C++" de l'atelier CRUD du tuteur). ---

    // SELECT de tous les employes, tries par ID. En cas d'erreur, renvoie une liste vide
    // et remplit *erreur (si fourni) avec le message Oracle.
    static QList<Employe> chargerTous(QString *erreur = nullptr);

    // Services proposes dans la fiche : SELECT ID, TYPE_SERVICE FROM SERVICE ORDER BY ID.
    // Colonne 0 = ID (a memoriser), colonne 1 = TYPE_SERVICE (a afficher). L'appelant prend
    // possession du modele (delete). En cas d'erreur, *erreur recoit un message lisible.
    static QSqlQueryModel *listerServices(QString *erreur = nullptr);

    // Types de service (contrainte CK_SERVICE_TYPE), dans l'ordre d'affichage du diagramme.
    static QStringList typesService();

    // Nombre de services de l'employe, par type (tous les types sont presents, a 0 si aucun
    // service). Lecture via EMPLOYE.ID_SERVICE. En cas d'erreur : tout a 0 et *erreur rempli.
    static QMap<QString, int> compterServicesParType(int idEmploye, QString *erreur = nullptr);

    // Qui occupe quel poste de travail en ce moment : POSTE_TRAVAIL -> employe (id, nom,
    // prenom et poste renseignes). Les postes absents de la table sont libres : on les
    // deduit en comparant avec la liste fixe PosteTravail::tous().
    static QMap<QString, Employe> occupantsParPoste(QString *erreur = nullptr);

    // Verifie l'identifiant/mot de passe (hachage inclus) ; si correct, remplit
    // employeTrouve et renvoie true. Ne renvoie jamais le mot de passe en clair.
    static bool authentifier(const QString &email, const QString &motDePasseClair,
                              Employe &employeTrouve, QString *erreur = nullptr);

    // INSERT (ajouter), UPDATE (modifier) ou DELETE (supprimer) de *this. ajouter()
    // met a jour m_id avec l'ID genere par la sequence Oracle en cas de succes.
    // En cas d'echec, *erreur recoit un message lisible (jamais d'erreur Oracle brute) et
    // *champ le nom du champ concerne s'il y en a un ("email", "telephone", "posteTravail"...),
    // pour que la Vue affiche le message sous le bon champ. Les regles de validation
    // ci-dessous sont controlees avant toute ecriture.
    bool ajouter(QString *erreur = nullptr, QString *champ = nullptr);
    bool modifier(QString *erreur = nullptr, QString *champ = nullptr);
    bool supprimer(QString *erreur = nullptr);

    // Un employe prend (ou libere, avec "") son propre poste de travail. Meme
    // pre-controle d'unicite que modifier() : echoue avec "Ce poste est déjà occupé par ..."
    // si un autre employe l'occupe. Met a jour m_posteTravail en cas de succes.
    bool changerPosteTravail(const QString &poste, QString *erreur = nullptr);

private:
    // Controle tous les champs ; false + message + nom du champ au premier probleme.
    bool valider(QString *erreur, QString *champ, bool motDePasseObligatoire) const;

    // true si un AUTRE employe occupe deja m_posteTravail (remplit *erreur avec son nom).
    bool posteDejaOccupe(QString *erreur) const;

    int m_id;
    QString m_nom;
    QString m_prenom;
    QString m_poste;
    QString m_email;
    QString m_telephone;
    QString m_statut;
    QString m_motDePasse;
    QString m_posteTravail;
    int m_idService;
};

#endif // EMPLOYE_H
