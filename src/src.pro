TEMPLATE = app

TARGET = qthttpd

QT = core network

INCLUDEPATH += ../http-parser

HEADERS = ../http-parser/http_parser.h \
    httpserver.h \
    httpworker.h \
    incomingconnectionevent.h

SOURCES = main.cpp \
    httpserver.cpp \
    httpworker.cpp \
    incomingconnectionevent.cpp

LIBS += -L../http-parser -lhttp-parser
QMAKE_RPATHDIR += ../http-parser

DEFINES += \
    QT_NO_CAST_FROM_ASCII \
    QT_NO_CAST_TO_ASCII \
    QT_NO_URL_CAST_FROM_STRING

#linux-g++:use_sendfile {
#    DEFINES += USE_SENDFILE
#}

CONFIG(release,debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}

#CONFIG(debug, debug|release) {
    QMAKE_CXXFLAGS += -pg
    QMAKE_LFLAGS += -pg
#}
