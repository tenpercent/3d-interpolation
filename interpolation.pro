CONFIG += warn_all debug_and_release
QT += opengl

debug {
	DEFINES += SAVE_TO_FILE
}

QMAKE_CXXFLAGS = -Wall -Wextra -std=c++0x

INCLUDEPATH += "include"
SOURCES += src/*
HEADERS += include/*

Release:DESTDIR = release
Release:OBJECTS_DIR = release/.obj
Release:MOC_DIR = release/.moc
Release:RCC_DIR = release/.rcc
Release:UI_DIR = release/.ui

Debug:DESTDIR = debug
Debug:OBJECTS_DIR = debug/.obj
Debug:MOC_DIR = debug/.moc
Debug:RCC_DIR = debug/.rcc
Debug:UI_DIR = debug/.ui
