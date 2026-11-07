#include "clientsmainwindow.h"
#include "connection.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Connection &c = Connection::createInstance();
    if (!c.createconnect()) {
        QMessageBox::critical(nullptr, "Base de données", "Échec de la connexion");
        return 1;
    }

    MainWindow w;
    w.show();
    const int resultat = a.exec();
    c.closeConnection();
    return resultat;
}