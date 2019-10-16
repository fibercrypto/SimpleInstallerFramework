#-------------------------------------------------
#
# Project created by QtCreator 2019-10-13T01:00:29
#
#
# SIF: Simple Installer Framework
# Simply, an alternative to Qt Installer Framework
#
# Copyright © 2019 Carlos Enrique Pérez Sánchez
#
# This project is under the GPLv3 license
#
#-------------------------------------------------

Qt += concurrent

TARGET = SimpleInstallerFramework
TEMPLATE = lib
CONFIG += staticlib

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        installer.cpp

HEADERS += \
        installer.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    CHANGELOG.md \
    LICENSE.GPLv3 \
    README.md
