#include "erreurbase.h"

#include <QDebug>
#include <QSqlError>

QString erreurLisible(const QSqlError &erreur, QString *champ)
{
    if (champ)
        champ->clear();

    // le nom de la contrainte (ex. UQ_EMPLOYE_EMAIL) se trouve dans le texte du pilote
    const QString texte = erreur.text() + " " + erreur.databaseText() + " " + erreur.driverText();
    qWarning() << "Erreur base de donnees :" << texte;   // jamais montre a l'utilisateur

    auto contient = [&texte](const char *motif) { return texte.contains(motif, Qt::CaseInsensitive); };
    auto pour = [champ](const char *nom) { if (champ) *champ = nom; };

    if (contient("ORA-00001")) {                       // violation d'unicite
        if (contient("UQ_EMPLOYE_EMAIL")) {
            pour("email");
            return "Cet email est déjà utilisé";
        }
        if (contient("UQ_EMPLOYE_POSTE_TRAVAIL")) {
            pour("posteTravail");
            return "Ce poste est déjà occupé par un autre employé";
        }
        return "Cette valeur existe déjà dans la base de données.";
    }

    if (contient("ORA-02290")) {                       // contrainte CHECK
        if (contient("CK_EMPLOYE_EMAIL"))        { pour("email");        return "L'email n'est pas valide"; }
        if (contient("CK_EMPLOYE_TEL"))          { pour("telephone");    return "Le téléphone doit contenir exactement 8 chiffres"; }
        if (contient("CK_EMPLOYE_PRENOM"))       { pour("prenom");       return "Le prénom contient des caractères non autorisés"; }
        if (contient("CK_EMPLOYE_NOM"))          { pour("nom");          return "Le nom contient des caractères non autorisés"; }
        if (contient("CK_EMPLOYE_STATUT"))       { pour("statut");       return "Veuillez choisir un statut"; }
        if (contient("CK_EMPLOYE_POSTE_TRAVAIL")){ pour("posteTravail"); return "Ce poste de travail n'est pas valide"; }
        if (contient("CK_EMPLOYE_POSTE"))        { pour("poste");        return "Veuillez choisir un poste"; }
        return "Une valeur saisie n'est pas acceptée.";
    }

    if (contient("ORA-12899"))
        return "Une des valeurs saisies est trop longue.";
    if (contient("ORA-01400")) {
        if (contient("ID_SERVICE") || contient("FK_EMPLOYE")) {
            pour("service");
            return "Veuillez choisir un service";
        }
        return "Un champ obligatoire est vide.";
    }
    if (contient("ORA-02292"))
        return "Impossible de supprimer cet élément : il est utilisé ailleurs dans l'application.";
    if (contient("ORA-02291")) {
        if (contient("ID_SERVICE") || contient("FK_EMPLOYE")) {
            pour("service");
            return "Ce service n'existe pas";
        }
        return "Cette valeur fait référence à un élément qui n'existe pas.";
    }
    if (contient("ORA-00942") || contient("ORA-00904"))
        return "La structure de la base de données n'est pas à jour. Contactez un administrateur.";

    if (erreur.type() == QSqlError::ConnectionError || contient("ORA-12541") || contient("ORA-01017")
        || contient("ORA-12514") || contient("data source name not found") || contient("ORA-03113")
        || contient("ORA-03114"))
        return "Connexion à la base de données impossible. Vérifiez que la base est démarrée.";

    return "Une erreur est survenue. Réessayez, ou contactez un administrateur si le problème continue.";
}
