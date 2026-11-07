#include "connection.h"
#include <QSqlError>
#include <QDebug>

Connection& Connection::createInstance()
{
    static Connection instance;            // Meyers Singleton
    return instance;
}

bool Connection::createconnect()
{
    if (!db.isValid())
        db = QSqlDatabase::addDatabase("QODBC");

    db.setDatabaseName("Source_Projet2A");
    db.setUserName("smartstitch");
    db.setPassword("smart123");


    if (db.open()) {
        qDebug() << "Connexion réussie";
        return true;
    }
    qDebug() << "Erreur de connexion :" << db.lastError().text();
    return false;
}

void Connection::closeConnection()
{
    db.close();
}