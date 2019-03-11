#-------------------------------------------------
#
# Project created by QtCreator 2017-12-13T15:58:44
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = DatabaseManager
TEMPLATE = app


SOURCES += main.cpp\
    databasemanager.cpp \
    widget.cpp \
    mythread.cpp \
    mythread2.cpp

HEADERS  += \
    databasemanager.h \
    widget.h \
    globalvar.h \
    mythread.h \
    mythread2.h

FORMS += \
    widget.ui
