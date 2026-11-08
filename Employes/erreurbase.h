#ifndef ERREURBASE_H
#define ERREURBASE_H

#include <QString>

class QSqlError;

// Traduit une erreur Oracle en message comprehensible par l'utilisateur : il ne doit jamais
// voir "ORA-xxxxx". Le texte brut est ecrit dans la sortie de l'application (pour le debogage).
// Si l'erreur concerne un champ precis (ex. email deja utilise), *champ recoit son nom
// ("email", "posteTravail", "telephone", "nom", "prenom", "poste", "statut"), pour que la Vue
// puisse afficher le message sous le bon champ ; sinon *champ est vide.
// Utilisee uniquement par les classes Modele (Employe, Service...).
QString erreurLisible(const QSqlError &erreur, QString *champ = nullptr);

#endif // ERREURBASE_H
