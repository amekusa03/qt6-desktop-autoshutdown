QT += widgets dbus

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    autoshutdowncore.cpp

HEADERS += \
    mainwindow.h \
    autoshutdowncore.h

TARGET = AutoShutdown
TEMPLATE = app
