#ifndef CONNECTION_H
#define CONNECTION_H

#include <QSqlDatabase>

class Connection
{
public:
    static Connection& createInstance();   // point d'accès unique
    bool createconnect();                  // ouvre la connexion
    void closeConnection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

private:
    Connection() {}                        // constructeur privé
    QSqlDatabase db;
};

#endif // CONNECTION_H