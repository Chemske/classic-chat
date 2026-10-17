APP_NAME = classicchat
TARGET = classicchat
CONFIG += qt warn_on cascades10
QT += network
INCLUDEPATH += $$PWD/src
SOURCES += src/main.cpp src/ChatController.cpp src/RelayReply.cpp
HEADERS += src/ChatController.hpp src/RelayReply.hpp
LIBS += -lbbdata -lbbsystem -lcurl -lssl -lcrypto
