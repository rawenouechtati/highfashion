QT += core gui widgets sql
CONFIG += c++17
TEMPLATE = app
TARGET = gestion-matieres

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    Stock.cpp \
    connection.cpp \
    Database.cpp \
    Fournisseur.cpp \
    Matiere.cpp \
    Pages.cpp

HEADERS += \
    mainwindow.h \
    Models.h \
    Stock.h \
    connection.h \
    Database.h \
    Fournisseur.h \
    Matiere.h \
    Pages.h

FORMS += mainwindow.ui
