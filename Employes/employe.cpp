#include "employe.h"
#include "motdepasseutils.h"
#include "erreurbase.h"
#include "posteTravail.h"

#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QDebug>
#include <QRegularExpression>
#include <QVariant>

Employe::Employe()
    : m_id(-1), m_statut("Actif"), m_idService(-1)
{
}

Employe::Employe(int id, const QString &nom, const QString &prenom, const QString &poste,
                  const QString &email, const QString &telephone,
                  const QString &statut, const QString &motDePasse,
                  const QString &posteTravail, int idService)
    : m_id(id), m_nom(nom), m_prenom(prenom), m_poste(poste), m_email(email),
      m_telephone(telephone), m_statut(statut), m_motDePasse(motDePasse),
      m_posteTravail(posteTravail), m_idService(idService)
{
}

int Employe::id() const { return m_id; }
void Employe::setId(int id) { m_id = id; }

QString Employe::nom() const { return m_nom; }
void Employe::setNom(const QString &nom) { m_nom = nom; }

QString Employe::prenom() const { return m_prenom; }
void Employe::setPrenom(const QString &prenom) { m_prenom = prenom; }

QString Employe::poste() const { return m_poste; }
void Employe::setPoste(const QString &poste) { m_poste = poste; }

QString Employe::email() const { return m_email; }
void Employe::setEmail(const QString &email) { m_email = email; }

QString Employe::telephone() const { return m_telephone; }
void Employe::setTelephone(const QString &telephone) { m_telephone = telephone; }

QString Employe::statut() const { return m_statut; }
void Employe::setStatut(const QString &statut) { m_statut = statut; }

QString Employe::motDePasse() const { return m_motDePasse; }
void Employe::setMotDePasse(const QString &motDePasse) { m_motDePasse = motDePasse; }

QString Employe::posteTravail() const { return m_posteTravail; }
void Employe::setPosteTravail(const QString &posteTravail) { m_posteTravail = posteTravail; }

int Employe::idService() const { return m_idService; }
void Employe::setIdService(int idService) { m_idService = idService; }

// --- Acces base de donnees ---------------------------------------------------------

QList<Employe> Employe::chargerTous(QString *erreur)
{
    QList<Employe> employes;

    QSqlQuery requete;
    if (!requete.exec("SELECT ID, NOM, PRENOM, POSTE, EMAIL, TELEPHONE, STATUT, MOT_DE_PASSE, "
                       "POSTE_TRAVAIL, ID_SERVICE FROM EMPLOYE ORDER BY ID")) {
        if (erreur)
            *erreur = erreurLisible(requete.lastError());
        return employes;
    }

    while (requete.next()) {
        const QVariant posteBrut = requete.value(8);
        employes.append(Employe(
            requete.value(0).toInt(),
            requete.value(1).toString(),
            requete.value(2).toString(),
            requete.value(3).toString(),
            requete.value(4).toString(),
            requete.value(5).toString(),
            requete.value(6).toString(),
            requete.value(7).toString(),
            posteBrut.toString(), // NULL -> chaine vide = aucun poste
            requete.value(9).isNull() ? -1 : requete.value(9).toInt()
            ));
    }
    return employes;
}

QMap<QString, Employe> Employe::occupantsParPoste(QString *erreur)
{
    QMap<QString, Employe> occupants;

    QSqlQuery requete;
    if (!requete.exec("SELECT POSTE_TRAVAIL, ID, NOM, PRENOM, POSTE FROM EMPLOYE "
                       "WHERE POSTE_TRAVAIL IS NOT NULL")) {
        if (erreur)
            *erreur = erreurLisible(requete.lastError());
        return occupants;
    }

    while (requete.next()) {
        const QString poste = requete.value(0).toString();
        Employe e;
        e.setId(requete.value(1).toInt());
        e.setNom(requete.value(2).toString());
        e.setPrenom(requete.value(3).toString());
        e.setPoste(requete.value(4).toString());
        e.setPosteTravail(poste);
        occupants.insert(poste, e);
    }
    return occupants;
}

bool Employe::authentifier(const QString &email, const QString &motDePasseClair,
                            Employe &employeTrouve, QString *erreur)
{
    QSqlQuery requete;
    requete.prepare("SELECT ID, NOM, PRENOM, POSTE, EMAIL, TELEPHONE, STATUT, MOT_DE_PASSE, "
                     "POSTE_TRAVAIL, ID_SERVICE FROM EMPLOYE WHERE LOWER(EMAIL) = LOWER(:email)");
    requete.bindValue(":email", email.trimmed()); // majuscules/minuscules sans importance pour l'identifiant

    if (!requete.exec()) {
        if (erreur)
            *erreur = erreurLisible(requete.lastError());
        return false;
    }

    if (!requete.next()) {
        // message generique cote Vue (on ne dit pas a l'utilisateur lequel des deux est faux),
        // mais la cause exacte est ecrite dans la sortie de l'application pour le debogage
        qWarning() << "Connexion refusee : aucun employe avec l'email" << email.trimmed();
        return false;
    }

    // Un hash hexadecimal s'ecrit en majuscules ou en minuscules selon l'outil qui l'a
    // produit (Oracle RAWTOHEX donne des majuscules, Qt des minuscules) : la casse ne doit
    // pas compter. On ne garde pas d'espaces parasites autour de la valeur stockee.
    const QString motDePasseHacheStocke = requete.value(7).toString().trimmed();
    if (hacherMotDePasse(motDePasseClair).compare(motDePasseHacheStocke, Qt::CaseInsensitive) != 0) {
        qWarning() << "Connexion refusee : mot de passe different pour" << email.trimmed()
                   << "- longueur en base :" << motDePasseHacheStocke.size()
                   << "(attendu 64 : empreinte SHA-256 en hexadecimal)";
        return false;
    }

    const QVariant posteBrut = requete.value(8);
    employeTrouve = Employe(
        requete.value(0).toInt(),
        requete.value(1).toString(),
        requete.value(2).toString(),
        requete.value(3).toString(),
        requete.value(4).toString(),
        requete.value(5).toString(),
        requete.value(6).toString(),
        motDePasseHacheStocke,
        posteBrut.toString(), // NULL -> chaine vide = aucun poste
        requete.value(9).isNull() ? -1 : requete.value(9).toInt()
        );
    return true;
}

// Pre-controle d'unicite : un poste de travail n'a qu'un occupant a la fois. Le message
// propre vient de ce SELECT (la contrainte UNIQUE en base reste le filet de securite).
// "m_id" est exclu pour qu'une modification ne se bloque pas sur l'employe lui-meme
// (en ajout m_id vaut -1, aucun employe n'a cet ID).
bool Employe::posteDejaOccupe(QString *erreur) const
{
    if (m_posteTravail.isEmpty())
        return false;

    QSqlQuery requete;
    requete.prepare("SELECT PRENOM, NOM FROM EMPLOYE WHERE POSTE_TRAVAIL = :poste AND ID != :monId");
    requete.bindValue(":poste", m_posteTravail);
    requete.bindValue(":monId", m_id);

    if (!requete.exec()) {
        if (erreur)
            *erreur = erreurLisible(requete.lastError());
        return true; // dans le doute on n'ecrit pas ; *erreur explique pourquoi
    }
    if (requete.next()) {
        if (erreur)
            *erreur = "Ce poste est déjà occupé par " + requete.value(0).toString()
                      + " " + requete.value(1).toString();
        return true;
    }
    return false;
}

bool Employe::ajouter(QString *erreur, QString *champ)
{
    if (!valider(erreur, champ, true))
        return false;

    // Un employe inactif ne garde pas de poste : il redevient libre.
    if (m_statut.compare("Inactif", Qt::CaseInsensitive) == 0)
        m_posteTravail.clear();

    if (posteDejaOccupe(erreur)) {
        if (champ)
            *champ = "posteTravail";
        return false;
    }

    QSqlQuery requete;
    // ID n'est pas insere : le trigger EMPLOYE_TRG le remplit (sequence EMPLOYE_SEQ).
    requete.prepare("INSERT INTO EMPLOYE (NOM, PRENOM, POSTE, EMAIL, TELEPHONE, STATUT, MOT_DE_PASSE, "
                     "POSTE_TRAVAIL, ID_SERVICE) "
                     "VALUES (:nom, :prenom, :poste, :email, :telephone, :statut, :motDePasse, "
                     ":posteTravail, :idService)");
    requete.bindValue(":idService", m_idService);
    // QVariant vide (type int) -> vrai NULL en base quand aucun poste n'est affecte
    requete.bindValue(":posteTravail", m_posteTravail.isEmpty() ? QVariant(QMetaType(QMetaType::QString))
                                                               : QVariant(m_posteTravail));
    requete.bindValue(":nom", m_nom);
    requete.bindValue(":prenom", m_prenom);
    requete.bindValue(":poste", m_poste);
    requete.bindValue(":email", m_email);
    requete.bindValue(":telephone", m_telephone);
    requete.bindValue(":statut", m_statut);
    requete.bindValue(":motDePasse", hacherMotDePasse(m_motDePasse)); // jamais en clair en base

    if (!requete.exec()) {
        if (erreur)
            *erreur = erreurLisible(requete.lastError(), champ);
        return false;
    }

    // Recupere l'ID genere par la sequence, pour que *this reste coherent avec la base.
    QSqlQuery requeteId;
    if (requeteId.exec("SELECT EMPLOYE_SEQ.CURRVAL FROM DUAL") && requeteId.next())
        m_id = requeteId.value(0).toInt();

    return true;
}

bool Employe::modifier(QString *erreur, QString *champ)
{
    if (!valider(erreur, champ, false))
        return false;

    // Un employe inactif ne garde pas de poste : il redevient libre, meme si le
    // formulaire affichait encore un poste.
    if (m_statut.compare("Inactif", Qt::CaseInsensitive) == 0)
        m_posteTravail.clear();

    if (posteDejaOccupe(erreur)) {
        if (champ)
            *champ = "posteTravail";
        return false;
    }

    QSqlQuery requete;
    if (!m_motDePasse.isEmpty()) {
        requete.prepare("UPDATE EMPLOYE SET NOM=:nom, PRENOM=:prenom, POSTE=:poste, EMAIL=:email, "
                         "TELEPHONE=:telephone, STATUT=:statut, POSTE_TRAVAIL=:posteTravail, "
                         "ID_SERVICE=:idService, MOT_DE_PASSE=:motDePasse WHERE ID=:id");
        requete.bindValue(":motDePasse", hacherMotDePasse(m_motDePasse)); // jamais en clair
    } else {
        // mot de passe laisse vide par l'appelant : on garde l'ancien mot de passe en base
        requete.prepare("UPDATE EMPLOYE SET NOM=:nom, PRENOM=:prenom, POSTE=:poste, EMAIL=:email, "
                         "TELEPHONE=:telephone, STATUT=:statut, POSTE_TRAVAIL=:posteTravail, "
                         "ID_SERVICE=:idService WHERE ID=:id");
    }
    // QVariant vide (type int) -> vrai NULL en base quand aucun poste n'est affecte
    requete.bindValue(":posteTravail", m_posteTravail.isEmpty() ? QVariant(QMetaType(QMetaType::QString))
                                                               : QVariant(m_posteTravail));
    requete.bindValue(":nom", m_nom);
    requete.bindValue(":prenom", m_prenom);
    requete.bindValue(":poste", m_poste);
    requete.bindValue(":email", m_email);
    requete.bindValue(":telephone", m_telephone);
    requete.bindValue(":statut", m_statut);
    requete.bindValue(":idService", m_idService);
    requete.bindValue(":id", m_id);

    if (!requete.exec()) {
        if (erreur)
            *erreur = erreurLisible(requete.lastError(), champ);
        return false;
    }
    return true;
}

bool Employe::supprimer(QString *erreur)
{
    QSqlQuery requete;
    requete.prepare("DELETE FROM EMPLOYE WHERE ID = :id");
    requete.bindValue(":id", m_id);

    if (!requete.exec()) {
        if (erreur)
            *erreur = erreurLisible(requete.lastError());
        return false;
    }
    return true;
}

bool Employe::changerPosteTravail(const QString &poste, QString *erreur)
{
    // Un employe inactif ne garde pas de poste (cf. modifier()).
    const QString nouveauPoste = m_statut.compare("Inactif", Qt::CaseInsensitive) == 0 ? QString() : poste;

    const QString ancienPoste = m_posteTravail;
    m_posteTravail = nouveauPoste;
    if (posteDejaOccupe(erreur)) {
        m_posteTravail = ancienPoste;
        return false;
    }

    QSqlQuery requete;
    requete.prepare("UPDATE EMPLOYE SET POSTE_TRAVAIL = :poste WHERE ID = :id");
    requete.bindValue(":poste", nouveauPoste.isEmpty() ? QVariant(QMetaType(QMetaType::QString))
                                                       : QVariant(nouveauPoste));
    requete.bindValue(":id", m_id);

    if (!requete.exec()) {
        m_posteTravail = ancienPoste;
        if (erreur)
            *erreur = erreurLisible(requete.lastError());
        return false;
    }
    return true;
}

// --- Regles de validation ----------------------------------------------------------

QStringList Employe::postes()
{
    return {"Gerant", "Couturier", "Vendeur", "Styliste"};   // = CHECK sur EMPLOYE.POSTE
}

QStringList Employe::statuts()
{
    return {"Actif", "Inactif", "Conge"};                    // = CHECK sur EMPLOYE.STATUT
}

namespace {

bool nomOuPrenomValide(const QString &texte, const QString &libelle, const QString &article,
                       QString *erreur)
{
    auto echec = [erreur](const QString &message) {
        if (erreur)
            *erreur = message;
        return false;
    };

    const QString propre = texte.trimmed();
    if (propre.isEmpty())
        return echec(article + libelle + " est obligatoire");
    if (propre.size() > 100)
        return echec(article + libelle + " ne doit pas dépasser 100 caractères");

    // lettres (accents compris), espaces, apostrophe, tiret
    static const QRegularExpression format(QStringLiteral("^[\\p{L} '\\-]+$"));
    if (!format.match(propre).hasMatch())
        return echec(article + libelle + " ne doit contenir que des lettres, espaces, apostrophes et tirets");
    return true;
}

} // namespace

bool Employe::nomValide(const QString &nom, QString *erreur)
{
    return nomOuPrenomValide(nom, "nom", "Le ", erreur);
}

bool Employe::prenomValide(const QString &prenom, QString *erreur)
{
    return nomOuPrenomValide(prenom, "prénom", "Le ", erreur);
}

bool Employe::telephoneValide(const QString &telephone, QString *erreur)
{
    auto echec = [erreur](const QString &message) {
        if (erreur)
            *erreur = message;
        return false;
    };
    if (telephone.trimmed().isEmpty())
        return echec("Le téléphone est obligatoire");
    static const QRegularExpression format(QStringLiteral("^\\d{8}$"));
    if (!format.match(telephone).hasMatch())
        return echec("Le téléphone doit contenir exactement 8 chiffres");
    return true;
}

bool Employe::emailValide(const QString &email, QString *erreur)
{
    auto echec = [erreur](const QString &message) {
        if (erreur)
            *erreur = message;
        return false;
    };
    const QString propre = email.trimmed();
    if (propre.isEmpty())
        return echec("L'email est obligatoire");
    if (propre.size() > 100)
        return echec("L'email ne doit pas dépasser 100 caractères");
    static const QRegularExpression format(
        QStringLiteral("^[A-Za-z0-9._%+\\-]+@[A-Za-z0-9.\\-]+\\.[A-Za-z]{2,}$"));
    if (!format.match(propre).hasMatch())
        return echec("L'email n'est pas valide (exemple : prenom.nom@exemple.com)");
    return true;
}

bool Employe::motDePasseValide(const QString &motDePasse, QString *erreur, bool obligatoire)
{
    if (motDePasse.isEmpty() && !obligatoire)
        return true;
    if (motDePasse.isEmpty()) {
        if (erreur)
            *erreur = "Le mot de passe est obligatoire";
        return false;
    }
    if (motDePasse.size() < 4) {
        if (erreur)
            *erreur = "Le mot de passe doit contenir au moins 4 caractères";
        return false;
    }
    return true;
}

bool Employe::posteValide(const QString &poste, QString *erreur)
{
    if (postes().contains(poste))
        return true;
    if (erreur)
        *erreur = "Veuillez choisir un poste";
    return false;
}

bool Employe::statutValide(const QString &statut, QString *erreur)
{
    if (statuts().contains(statut))
        return true;
    if (erreur)
        *erreur = "Veuillez choisir un statut";
    return false;
}

bool Employe::posteTravailValide(const QString &poste, QString *erreur)
{
    if (poste.isEmpty() || PosteTravail::estValide(poste))
        return true;
    if (erreur)
        *erreur = "Ce poste de travail n'est pas valide";
    return false;
}

QStringList Employe::typesService()
{
    return {"Location", "Restauration", "Nettoyage", "Authentification"};
}

QMap<QString, int> Employe::compterServicesParType(int idEmploye, QString *erreur)
{
    QMap<QString, int> nombreParType;
    for (const QString &type : typesService())
        nombreParType[type] = 0;

    QSqlQuery requete;
    requete.prepare("SELECT S.TYPE_SERVICE, COUNT(*) FROM EMPLOYE E "
                    "JOIN SERVICE S ON S.ID = E.ID_SERVICE "
                    "WHERE E.ID = :id GROUP BY S.TYPE_SERVICE");
    requete.bindValue(":id", idEmploye);

    if (!requete.exec()) {
        if (erreur)
            *erreur = erreurLisible(requete.lastError());
        return nombreParType;
    }

    while (requete.next())
        nombreParType[requete.value(0).toString()] = requete.value(1).toInt();
    return nombreParType;
}

bool Employe::serviceValide(const QString &idService, QString *erreur)
{
    bool ok = false;
    if (idService.toInt(&ok) > 0 && ok)
        return true;
    if (erreur)
        *erreur = "Veuillez choisir un service";
    return false;
}

QSqlQueryModel *Employe::listerServices(QString *erreur)
{
    auto *modele = new QSqlQueryModel();
    modele->setQuery("SELECT ID, TYPE_SERVICE FROM SERVICE ORDER BY ID");
    if (modele->lastError().isValid() && erreur)
        *erreur = erreurLisible(modele->lastError());
    return modele;
}

bool Employe::valider(QString *erreur, QString *champ, bool motDePasseObligatoire) const
{
    // Meme controle que la Vue a la frappe : ici c'est le filet de securite avant l'ecriture.
    struct Controle { const char *champ; bool ok; QString message; };
    QString message;
    auto verifie = [&](const char *nomChamp, bool ok) {
        if (ok)
            return true;
        if (erreur)
            *erreur = message;
        if (champ)
            *champ = nomChamp;
        return false;
    };
    if (champ)
        champ->clear();

    return verifie("nom", nomValide(m_nom, &message))
        && verifie("prenom", prenomValide(m_prenom, &message))
        && verifie("poste", posteValide(m_poste, &message))
        && verifie("statut", statutValide(m_statut, &message))
        && verifie("email", emailValide(m_email, &message))
        && verifie("telephone", telephoneValide(m_telephone, &message))
        && verifie("posteTravail", posteTravailValide(m_posteTravail, &message))
        && verifie("service", serviceValide(QString::number(m_idService), &message))
        && verifie("motDePasse", motDePasseValide(m_motDePasse, &message, motDePasseObligatoire));
}
