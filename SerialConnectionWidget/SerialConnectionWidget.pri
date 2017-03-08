!contains( included_modules, $$PWD ) {
    included_modules += $$PWD
QT += core gui

DEPENDPATH += "$$PWD/src"
INCLUDEPATH += "$$PWD/src"

SOURCES += $$PWD/src/SerialConnectionWidget.cpp

HEADERS  += $$PWD/src/SerialConnectionWidget.h

FORMS += $$PWD/src/SerialConnectionWidgetForm.ui

RESOURCES += $$PWD/SerialConnectionWidget.qrc
}
