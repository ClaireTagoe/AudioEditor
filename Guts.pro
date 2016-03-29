#-------------------------------------------------
#
# Project created by QtCreator 2016-03-02T20:05:58
#
#-------------------------------------------------

QT       += core gui
QT += multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Guts
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h \
    sndfile.h \
    sndfile.hh

FORMS    += mainwindow.ui

CONFIG += C++11


macx: LIBS += -L$$PWD/./ -lsndfile

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

macx: PRE_TARGETDEPS += $$PWD/./libsndfile.a
