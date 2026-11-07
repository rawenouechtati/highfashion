#include "connection.h"
#include <QSqlError>
#include <QDebug>

Connection& Connection::createInstance()
{
    static Connection instance;
    return instance;
}

bool Connection::createconnect()
{
    db = QSqlDatabase::addDatabase("QODBC");
    db.setDatabaseName("Projet2A25");
    db.setUserName("smartstitch");
    db.setPassword("smart123");

    if (db.open()) {
        qDebug() << "Connexion reussie";
        return true;
    }
    qDebug() << "Echec :" << db.lastError().text();
    return false;
}