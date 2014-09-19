CONFIG += warn_on
# CONFIG += debug_and_release_target
# CONFIG += debug
CONFIG += build_all
QT += opengl

INCLUDEPATH += "include"
SOURCES += src/*
HEADERS += include/*

CONFIG(debug, debug|release) {
    DESTDIR = build/debug
    OBJECTS_DIR = build/debug/.obj
    MOC_DIR = build/debug/.moc
    RCC_DIR = build/debug/.rcc
    UI_DIR = build/debug/.ui
} else {
    DESTDIR = build/release
    OBJECTS_DIR = build/release/.obj
    MOC_DIR = build/release/.moc
    RCC_DIR = build/release/.rcc
    UI_DIR = build/release/.ui
}
