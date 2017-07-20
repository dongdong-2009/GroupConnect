#-------------------------------------------------
#
# Project created by QtCreator 2017-07-15T10:41:57
#
#-------------------------------------------------

QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = chat
TEMPLATE = app


SOURCES += main.cpp\
        widget.cpp \
    tcpserver.cpp \
    tcpclient.cpp \
    login.cpp \
    chat.cpp

HEADERS  += widget.h \
    tcpserver.h \
    tcpclient.h \
    login.h \
    chat.h

FORMS    += widget.ui \
    tcpserver.ui \
    tcpclient.ui \
    login.ui \
    chat.ui

RESOURCES += \
    imgs.qrc

RC_ICONS = imgs/QtChat.ico
