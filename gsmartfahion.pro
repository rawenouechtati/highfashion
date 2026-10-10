QT += widgets charts

CONFIG += c++17

SOURCES += \
    main.cpp \
    gemployes.cpp \
    mainwindow.cpp \
    mainwindow1.cpp \
    notificationsdialog.cpp

HEADERS += \
    gemployes.h \
    mainwindow.h \
    mainwindow1.h \
    notificationsdialog.h

FORMS += \
    gemployes.ui \
    mainwindow.ui \
    mainwindow1.ui \
    notificationsdialog.ui

RESOURCES += \
    resources.qrc

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
