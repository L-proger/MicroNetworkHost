SOURCES += $$files($$PWD/src/*.cpp, true) \
    $$PWD/src/MicroNetwork/Host/TaskContext.cpp
HEADERS += $$files($$PWD/src/*.h, true) \
    $$PWD/src/MicroNetwork/Host/INetwork.h \
    $$PWD/src/MicroNetwork/Host/LinkProvider.h \
    $$PWD/src/MicroNetwork/Host/NodeContext.h \
    $$PWD/src/MicroNetwork/Host/TaskContext.h \
    $$PWD/src/MicroNetwork/Host/UsbLinkProvider.h
INCLUDEPATH += $$PWD/Src
