#ifndef CLIENT_H
#define CLIENT_H

#include <QString>
#include <QSqlQuery>

class Client
{
public:
    Client() {}
    explicit Client(int id) : id(id) {}
    Client(int id, const QString &nom, const QString &email,
           const QString &tel, const QString &cin);

    bool ajouter();
    bool modifier();
    bool supprimer();
    static bool afficher(QSqlQuery &q);

    QString derniereErreur() const { return erreur; }

private:
    int id = 0;
    QString nom, email, tel, cin;
    QString erreur;
};

#endif // CLIENT_H