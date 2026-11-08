cmake_minimum_required(VERSION 3.16)

project(highfashion VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    notificationsdialog.cpp
    notificationsdialog.h
    notificationsdialog.ui
    connection.h connection.cpp
    service.h service.cpp
    client.h client.cpp
    resources.qrc
    LOGO.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(highfashion
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(highfashion ${PROJECT_SOURCES})
endif()

target_link_libraries(highfashion PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
)
set_target_properties(highfashion PROPERTIES
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(highfashion)
endif()