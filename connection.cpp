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
    db.setDatabaseName("Source_Projet2A");
    db.setUserName("rawen");
    db.setPassword("projet12");

    if (db.open()) {
        qDebug() << "Connexion reussie";
        return true;
    }
    qDebug() << "Echec :" << db.lastError().text();
    return false;
}