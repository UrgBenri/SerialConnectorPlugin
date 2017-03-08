TEMPLATE    = lib
CONFIG      += plugin
DESTDIR     = $$PWD/../bin/plugins
CONFIG      += c++11
TARGET      = SerialConnectorPlugin

!include(SerialConnectorPlugin.pri) {
        error("Unable to include Serial Connector Plugin.")
}

CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT
