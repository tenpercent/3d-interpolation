CONFIG += warn_all debug
QT += opengl

debug {
	DEFINES += SAVE_TO_FILE
}

QMAKE_CXXFLAGS += -Wextra 

TEMPLATE = subdirs
SUBDIRS += src include

