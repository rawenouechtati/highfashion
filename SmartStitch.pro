QT       += core gui widgets sql charts printsupport network
CONFIG   += c++17
TARGET    = SmartStitch
TEMPLATE  = app

INCLUDEPATH += $$PWD $$PWD/Maysam_Module

# Fichiers à la racine : main.cpp, mainwindow.*, connection.*, service.*...
SOURCES += $$files($$PWD/*.cpp)
HEADERS += $$files($$PWD/*.h)
FORMS   += $$files($$PWD/*.ui)

# Module Clients (Isra) : fichiers listés un par un
SOURCES += \
    Maysam_Module/client.cpp \
    Maysam_Module/clientsmainwindow.cpp
HEADERS += \
    Maysam_Module/client.h \
    Maysam_Module/clientsmainwindow.h
FORMS += \
    Maysam_Module/clientsmainwindow.ui

RESOURCES += resources.qrc

# === MODULE EMPLOYE ===
INCLUDEPATH += $$PWD/Employes
SOURCES += $$files($$PWD/Employes/*.cpp)
HEADERS += $$files($$PWD/Employes/*.h)
RESOURCES += Employes/ressources.qrc