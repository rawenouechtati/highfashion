QT += widgets charts

CONFIG += c++17

<<<<<<< HEAD:gestion-clients.pro

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main2.cpp \
    clientsmainwindow.cpp

HEADERS += \
    clientsmainwindow.h

FORMS += \
    clientsmainwindow.ui
=======
SOURCES += \
    main.cpp \
    gemployes.cpp \
    mainwindow.cpp \
    clientsmainwindow.cpp \
    gevenements.cpp \
    evolutionchart.cpp \
    notificationsdialog.cpp

HEADERS += \
    gemployes.h \
    mainwindow.h \
    clientsmainwindow.h \
    gevenements.h \
    evolutionchart.h \
    notificationsdialog.h

FORMS += \
    gemployes.ui \
    mainwindow.ui \
    clientsmainwindow.ui \
    gevenements.ui \
    notificationsdialog.ui

RESOURCES += \
    resources.qrc
>>>>>>> origin/integration:gsmartfahion.pro

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    LOGO.qrc
