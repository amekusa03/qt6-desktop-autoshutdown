QT += widgets dbus network

CONFIG += c++17

SOURCES +=     main.cpp     mainwindow.cpp     autoshutdowncore.cpp     shutdowndialog.cpp

HEADERS +=     mainwindow.h     autoshutdowncore.h     shutdowndialog.h

TARGET = AutoShutdown
TEMPLATE = app
