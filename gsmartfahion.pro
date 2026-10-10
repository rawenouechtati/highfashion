QT += widgets charts

CONFIG += c++17

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

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
