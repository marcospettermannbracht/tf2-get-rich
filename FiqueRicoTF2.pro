#-------------------------------------------------
#
# Project created by QtCreator 2014-11-08T01:48:18
#
#-------------------------------------------------

QT       += core gui multimedia
QT       += webkitwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FiqueRicoTF2
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    tf2search.cpp \
    janelaaddlink.cpp \
    loghelper.cpp

HEADERS  += mainwindow.h \
    tf2search.h \
    janelaaddlink.h \
    persistentcookiejar.h \
    loghelper.h

FORMS    += mainwindow.ui \
    janelaaddlink.ui

RESOURCES += \
    Icones.qrc \
    sons.qrc
