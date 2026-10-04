#include <QApplication>
#include <QFile>
#include <QStyleFactory>
#include <QFontDatabase>
#include <QIcon>
#include <QDebug>
#include "gemployes.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));
    QFile fichierStyle(":/style.qss");
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

    GEmployes w;
    w.show();
    return a.exec();
}