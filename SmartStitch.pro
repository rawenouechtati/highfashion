QT       += core gui widgets sql charts printsupport network
# Standard C++ : C++11 avec Qt 5. Qt 6 EXIGE au minimum C++17 (ses en-tetes ne compilent pas en C++11),
# donc on passe a c++17 seulement si Qt 6 est utilise. Le code du projet, lui, reste ecrit en C++11.
greaterThan(QT_MAJOR_VERSION, 5) {
    CONFIG += c++17
} else {
    CONFIG += c++11
}
TARGET    = SmartStitch
TEMPLATE  = app

INCLUDEPATH += $$PWD

# Fichiers à la racine : main.cpp, mainwindow.*, connection.*, service.*...
SOURCES += $$files($$PWD/*.cpp)
HEADERS += $$files($$PWD/*.h)
FORMS   += $$files($$PWD/*.ui)

RESOURCES += resources.qrc

# === MODULE SERVICE ===
# Corrige "service.h: No such file or directory" : service.h/.cpp peuvent etre a la racine
# OU dans un sous-dossier (Services/, Service/...). Si absent de la racine, on le cherche partout.
INCLUDEPATH += $$PWD/Services $$PWD/Service
SOURCES += $$files($$PWD/Services/*.cpp) $$files($$PWD/Service/*.cpp)
HEADERS += $$files($$PWD/Services/*.h)   $$files($$PWD/Service/*.h)
!exists($$PWD/service.h):!exists($$PWD/Services/service.h):!exists($$PWD/Service/service.h) {
    SERVICE_H = $$files($$PWD/service.h, true)
    isEmpty(SERVICE_H) {
        error("service.h introuvable dans $$PWD : recuperez le module Service (git pull / git checkout main -- service.h service.cpp)")
    }
    SERVICE_DIR = $$dirname($$first(SERVICE_H))
    INCLUDEPATH += $$SERVICE_DIR
    SOURCES += $$files($$SERVICE_DIR/*.cpp)
    HEADERS += $$files($$SERVICE_DIR/*.h)
}

# === MODULE EMPLOYE ===
INCLUDEPATH += $$PWD/Employes
SOURCES += $$files($$PWD/Employes/*.cpp)
HEADERS += $$files($$PWD/Employes/*.h)
RESOURCES += Employes/ressources.qrc

# === MODULE CLIENT ===
INCLUDEPATH += $$PWD/Clients
SOURCES += $$files($$PWD/Clients/*.cpp)
HEADERS += $$files($$PWD/Clients/*.h)
RESOURCES += Clients/ressources_clients.qrc

# === MODULE EVENEMENT ===
   INCLUDEPATH += $$PWD/Evenements
   SOURCES += $$files($$PWD/Evenements/*.cpp)
   HEADERS += $$files($$PWD/Evenements/*.h)
   RESOURCES += Evenements/ressources_evenements.qrc

# === MODULE ARTICLE ===
INCLUDEPATH += $$PWD/Articles
SOURCES += $$files($$PWD/Articles/*.cpp)
HEADERS += $$files($$PWD/Articles/*.h)

# === MODULE MATIERE PREMIERE ===
INCLUDEPATH += $$PWD/Matieres
SOURCES += $$files($$PWD/Matieres/*.cpp)
HEADERS += $$files($$PWD/Matieres/*.h)
