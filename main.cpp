#include <QApplication>
#include <QStyleFactory>
#include <QFontDatabase>
#include <QIcon>
#include <QFile>
#include <QDebug>
#include "gemployes.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));

    const QStringList polices = {
        ":/fonts/PlayfairDisplay-Regular.ttf",
        ":/fonts/PlayfairDisplay-Bold.ttf",
        ":/fonts/Lato-Regular.ttf",
        ":/fonts/Lato-Bold.ttf"
    };
    for (const QString &chemin : polices) {
        if (QFontDatabase::addApplicationFont(chemin) == -1)
            qWarning() << "Police introuvable :" << chemin;
    }

    QFile fichierStyle(":/style.qss");
    if (fichierStyle.open(QFile::ReadOnly | QFile::Text)) {
        a.setStyleSheet(fichierStyle.readAll());
        fichierStyle.close();
    }

    a.setFont(QFont("Lato", 10));
    a.setWindowIcon(QIcon(":/images/icone.png"));

    GEmployes w;
    w.show();
    return a.exec();
}
