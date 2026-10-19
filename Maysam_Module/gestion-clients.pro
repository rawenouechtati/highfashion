QT += core gui widgets printsupport sql
CONFIG += c++17
# CONFIG += console   # facultatif : pour voir qDebug dans une console

SOURCES += \
    client.cpp \
    main2.cpp \
    clientsmainwindow.cpp \
    connection.cpp

HEADERS += \
    client.h \
    clientsmainwindow.h \
    connection.h

FORMS += \
    clientsmainwindow.ui

RESOURCES += \
    LOGO.qrc