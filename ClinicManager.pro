QT += core gui widgets sql
CONFIG += c++17
TEMPLATE = app
TARGET = ClinicManager

SOURCES += \
    main.cpp \
    databasemanager.cpp \
    loginwidget.cpp \
    mainwindow.cpp \
    patientdialog.cpp

HEADERS += \
    databasemanager.h \
    loginwidget.h \
    mainwindow.h \
    patientdialog.h

