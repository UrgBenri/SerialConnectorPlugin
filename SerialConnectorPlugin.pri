!contains( included_modules, $$PWD ) {
    included_modules += $$PWD
    QT += core gui widgets
    win32:LIBS += -luser32

    !include($$PWD/PluginInterface/UrgBenriPluginInterface.pri) {
            error("Unable to include Viewer Plugin Interface.")
    }

    !include($$PWD/ModeSwitcherWidget/ModeSwitcherWidget.pri) {
            error("Unable to include Mode Switcher Widget.")
    }

    !include($$PWD/QUrgLib/QUrgLib.pri) {
            error("Unable to include QUrg Library.")
    }

    !include($$PWD/SerialConnectionWidget/SerialConnectionWidget.pri) {
            error("Unable to include Serial Connection Widget.")
    }

    !include($$PWD//RangeViewWidget/RangeViewWidget.pri) {
            error("Unable to include Range View Widget.")
    }

    DEPENDPATH += $$PWD
    INCLUDEPATH += $$PWD

    SOURCES += \
            $$PWD/SerialConnectorPlugin.cpp \
            $$PWD/SerialConnectionCheck.cpp \
            $$PWD/SerialDataConverter.cpp \
            $$PWD/SerialSettingsForm.cpp

    HEADERS  += \
            $$PWD/SerialConnectorPlugin.h \
            $$PWD/SerialConnectionCheck.h \
            $$PWD/SerialSettingsForm.h \
            $$PWD/SerialDataConverter.h

    FORMS    += \
            $$PWD/SerialConnectorPlugin.ui \
            $$PWD/SerialSettingsForm.ui

    RESOURCES += \
            $$PWD/SerialConnectorPlugin.qrc

    TRANSLATIONS = $$PWD/i18n/plugin_fr.ts \
            $$PWD/i18n/plugin_en.ts \
            $$PWD/i18n/plugin_ja.ts
}
