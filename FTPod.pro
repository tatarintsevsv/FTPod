#-------------------------------------------------
#
# Project created by QtCreator 2021-08-25T11:43:00
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FTPod
TEMPLATE = app

# libCurl library
LIBS += "C:/Program Files/OpenSSL/bin/libcurl.dll"

# libCurl include dir
INCLUDEPATH += C:/MAIN64/lib/include

SOURCES += main.cpp\
        mainwindow.cpp \
    ftp.cpp \
    ftpparse.c

HEADERS  += mainwindow.h \
    ftp.h \
    ftpparse.h

FORMS    += mainwindow.ui
