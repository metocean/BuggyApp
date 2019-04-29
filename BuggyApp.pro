TEMPLATE = app
CONFIG += console c++1z
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS_RELEASE += -O3
QMAKE_CFLAGS_RELEASE   += -O3
QMAKE_LFLAGS_RELEASE   += -Wl,-O3

LIBS += -L/usr/local/lib -lpthread

SOURCES += \
        main.cpp \
    grody/picohttpparser/picohttpparser.c \
    grody/webserver.c \
    grody/thread.c \
    grody/io.c \
    grody/fork.c

HEADERS += \
    grody/picohttpparser/picohttpparser.h \
    grody/threaded_webserver.h \
    grody/webserver.h \
    grody/thread.h \
    grody/fork.h
