#include <QApplication>
#include <QFile>
#include <QStyleFactory>
#include <QFontDatabase>
#include <QIcon>
#include <QDebug>
#include <QMessageBox>
#include "mainwindow.h"
#include "connexion.h"
#include "connection.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("SmartStitch");
    a.setApplicationName("SmartStitch");

    // Connexion Oracle commune a toute l'equipe (DSN Projet2A25), ouverte une seule fois au demarrage.
    if (!Connection::createInstance().createconnect()) {
        QMessageBox::critical(nullptr, QObject::tr("Base de données"),
                              QObject::tr("Échec de connexion à la base Oracle.\n"
                                          "Vérifiez le DSN Projet2A25 puis relancez l'application."));
        return 1;
    }

    a.setStyle(QStyleFactory::create("Fusion"));
    // Feuille de style du module Employes (declaree sous l'alias style_employes.qss dans
    // Employes/ressources.qrc, pour ne pas entrer en conflit avec le style.qss de la racine).
    QFile fichierStyle(":/style_employes.qss");
    if (!fichierStyle.exists())
        fichierStyle.setFileName(":/style.qss");
    if (fichierStyle.open(QFile::ReadOnly | QFile::Text)) {
        a.setStyleSheet(fichierStyle.readAll());
        fichierStyle.close();
    }

    const QStringList polices = {
        ":/fonts/PlayfairDisplay-Regular.ttf",
        ":/fonts/PlayfairDisplay-Bold.ttf",
        ":/fonts/Lato-Regular.ttf",
        ":/fonts/Lato-Bold.ttf"
    };
    for (const QString &chemin : polices) {
        int id = QFontDatabase::addApplicationFont(chemin);
        if (id == -1)
            qWarning() << "Police introuvable :" << chemin;
        else
            qDebug() << chemin << "->" << QFontDatabase::applicationFontFamilies(id);
    }

    a.setFont(QFont("Lato", 10));
    a.setWindowIcon(QIcon(":/images/icone.png"));

    // Boucle connexion -> fenetre principale : "Deconnexion" ferme la fenetre et rouvre l'ecran
    // de connexion ; fermer la fenetre autrement quitte l'application.
    while (true) {
        Connexion fenetreConnexion;
        if (fenetreConnexion.exec() != QDialog::Accepted)
            return 0;

        // On transmet l'employe connecte : la page Employes affiche la gestion complete pour un
        // Gerant, ou "Mon espace" pour les autres postes.
        MainWindow w(fenetreConnexion.employeConnecte());
        bool deconnexion = false;
        QObject::connect(&w, &MainWindow::deconnexionDemandee, &w, [&]() {
            deconnexion = true;
            w.close();
        });
        w.showMaximized();
        a.exec();
        if (!deconnexion)
            return 0;
    }
}
