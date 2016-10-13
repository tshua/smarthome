#-------------------------------------------------
#
# Project created by QtCreator 2016-10-09T10:18:36
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = smart_home
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    common.cpp \
    protocol.cpp \
    recvmsgthread.cpp

HEADERS  += mainwindow.h \
    common.h \
    protocol.h \
    recvmsgthread.h

RESOURCES += \
    images/img.qrc
