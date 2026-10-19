#ifndef CONNECTION_H
#define CONNECTION_H

#include <QSqlDatabase>

class Connection
{
public:
    static Connection& createInstance();
    bool createconnect();
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

private:
    Connection() {}
    QSqlDatabase db;
};

#endif
