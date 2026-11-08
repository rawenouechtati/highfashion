QT       += core gui widgets sql charts printsupport network
CONFIG   += c++17
TARGET    = SmartStitch
TEMPLATE  = app

INCLUDEPATH += $$PWD

# Fichiers à la racine : main.cpp, mainwindow.*, connection.*, service.*, client.*
SOURCES += $$files($$PWD/*.cpp)
HEADERS += $$files($$PWD/*.h)
FORMS   += $$files($$PWD/*.ui)

RESOURCES += resources.qrc LOGO.qrc