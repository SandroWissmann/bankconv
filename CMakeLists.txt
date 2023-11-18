cmake_minimum_required(VERSION 3.16)

project(bankconv VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.6 COMPONENTS Core Quick Widgets REQUIRED)

qt_add_executable(app ressource.qrc Main.cpp)

qt_add_qml_module(app
    VERSION 1.0
    URI bankconv
    QML_FILES
        Main.qml
    SOURCES
        Backend.cpp
        Backend.hpp
        EntryCheckingAccount.hpp
        EntryCheckingAccount.cpp
        EntryCreditCard.hpp
        EntryCreditCard.cpp
        EntryUtils.hpp
        EntryUtils.cpp
)

target_link_libraries(app PRIVATE Qt6::Quick Qt6::Widgets)

set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bankconv)
set(QML_IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY} CACHE STRING "Import paths for Qt Creator's code model" FORCE)
