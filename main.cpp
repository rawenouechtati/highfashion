#include "mainwindow.h"
#include "connection.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    bool test = Connection::createInstance().createconnect();

    MainWindow w;
    if (test)
    {
        w.show();
        QMessageBox::information(nullptr, QObject::tr("Base de données"),
                                 QObject::tr("Connexion réussie.\nClick Cancel to exit."),
                                 QMessageBox::Cancel);
    }
    else
    {
        QMessageBox::critical(nullptr, QObject::tr("Base de données"),
                              QObject::tr("Échec de connexion.\nClick Cancel to exit."),
                              QMessageBox::Cancel);
    }

    return a.exec();
}