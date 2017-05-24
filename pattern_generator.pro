QT += core gui widgets

TARGET = pattern_generator
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += main.cpp \
    worker.cpp \
    widget.cpp

HEADERS += \
    worker.h \
    widget.h
