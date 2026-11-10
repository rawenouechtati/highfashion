#include "client.h"
#include <QSqlError>
#include <QRegularExpression>

Client::Client(int id, const QString &nom, const QString &prenom, const QString &email,
               const QString &tel, const QString &niveau, const QString &statut)
    : id(id), nom(nom.trimmed()), prenom(prenom.trimmed()), email(email.trimmed()),
      tel(tel.trimmed()), niveau(niveau), statut(statut)
{
}

QStringList Client::niveauxAutorises() { return {"Bronze", "Argent", "Or"}; }
QStringList Client::statutsAutorises() { return {"Actif", "Inactif"}; }

bool Client::valider(QString *message) const
{
    auto refuser = [message](const QString &m) {
        if (message) *message = m;
        return false;
    };

    static const QRegularExpression reNom(R"(^[\p{L} '\-]+$)");
    static const QRegularExpression reEmail(R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");
    static const QRegularExpression reTel(R"(^[0-9 ]{8,12}$)");

    if (nom.isEmpty() || prenom.isEmpty() || email.isEmpty() || tel.isEmpty())
        return refuser("Veuillez remplir tous les champs.");
    if (nom.toUtf8().size() > 100 || !reNom.match(nom).hasMatch())
        return refuser("Le nom ne doit contenir que des lettres (100 caractères maximum).");
    if (prenom.toUtf8().size() > 100 || !reNom.match(prenom).hasMatch())
        return refuser("Le prénom ne doit contenir que des lettres (100 caractères maximum).");
    if (email.toUtf8().size() > 100 || !reEmail.match(email).hasMatch())
        return refuser("Email invalide (ex : nom@domaine.com).");
    if (!reTel.match(tel).hasMatch())
        return refuser("Le téléphone doit contenir entre 8 et 12 chiffres.");
    if (!niveauxAutorises().contains(niveau))
        return refuser("Niveau invalide (Bronze, Argent ou Or).");
    if (!statutsAutorises().contains(statut))
        return refuser("Statut invalide (Actif ou Inactif).");
    return true;
}

bool Client::ajouter()
{
    // l'ID vient de la sequence CLIENT_SEQ
    QSqlQuery q;
    q.prepare("INSERT INTO client (id, nom, prenom, email, telephone, niveau, statut) "
              "VALUES (CLIENT_SEQ.NEXTVAL, :nom, :prenom, :email, :tel, :niveau, :statut)");
    q.bindValue(":nom", nom);
    q.bindValue(":prenom", prenom);
    q.bindValue(":email", email);
    q.bindValue(":tel", tel);
    q.bindValue(":niveau", niveau);
    q.bindValue(":statut", statut);
    if (!q.exec()) {
        erreur = q.lastError().text();
        return false;
    }
    return true;
}

bool Client::modifier()
{
    QSqlQuery q;
    q.prepare("UPDATE client SET nom=:nom, prenom=:prenom, email=:email, telephone=:tel, "
              "niveau=:niveau, statut=:statut WHERE id=:id");
    q.bindValue(":nom", nom);
    q.bindValue(":prenom", prenom);
    q.bindValue(":email", email);
    q.bindValue(":tel", tel);
    q.bindValue(":niveau", niveau);
    q.bindValue(":statut", statut);
    q.bindValue(":id", id);
    if (!q.exec()) {
        erreur = q.lastError().text();
        return false;
    }
    return true;
}

bool Client::supprimer()
{
    QSqlQuery q;
    q.prepare("DELETE FROM client WHERE id=:id");
    q.bindValue(":id", id);
    if (!q.exec()) {
        erreur = q.lastError().text();
        return false;
    }
    return true;
}

bool Client::afficher(QSqlQuery &q)
{
    return q.exec("SELECT id, nom, prenom, email, telephone, niveau, statut "
                  "FROM client ORDER BY id");
}
