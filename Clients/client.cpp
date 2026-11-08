#include "client.h"
#include <QSqlError>

Client::Client(int id, const QString &nom, const QString &email,
               const QString &tel, const QString &cin)
    : id(id), nom(nom), email(email), tel(tel), cin(cin)
{
}

bool Client::ajouter()
{
    QSqlQuery q;
    q.prepare("INSERT INTO client (nom, email, telephone, cin) "
              "VALUES (:nom, :email, :tel, :cin)");
    q.bindValue(":nom", nom);
    q.bindValue(":email", email);
    q.bindValue(":tel", tel);
    q.bindValue(":cin", cin);
    if (!q.exec()) {
        erreur = q.lastError().text();
        return false;
    }
    return true;
}

bool Client::modifier()
{
    QSqlQuery q;
    q.prepare("UPDATE client SET nom=:nom, email=:email, "
              "telephone=:tel, cin=:cin WHERE id=:id");
    q.bindValue(":nom", nom);
    q.bindValue(":email", email);
    q.bindValue(":tel", tel);
    q.bindValue(":cin", cin);
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
    return q.exec("SELECT id, nom, email, telephone, cin, points, niveau "
                  "FROM client ORDER BY id");
}