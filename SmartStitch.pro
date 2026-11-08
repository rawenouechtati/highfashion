QT       += core gui widgets sql charts printsupport network
CONFIG   += c++17
TARGET    = SmartStitch
TEMPLATE  = app

INCLUDEPATH += $$PWD

# Fichiers à la racine : main.cpp, mainwindow.*, connection.*, service.*...
SOURCES += $$files($$PWD/*.cpp)
HEADERS += $$files($$PWD/*.h)
FORMS   += $$files($$PWD/*.ui)

RESOURCES += resources.qrc

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