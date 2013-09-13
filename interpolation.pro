CONFIG += warn_all debug
QT += opengl

debug {
	DEFINES += SAVE_TO_FILE
}

QMAKE_CXXFLAGS += -Wextra 

SOURCES += window.cpp function.cpp 3d.cpp msr.cpp threads.cpp integrals.cpp main.cpp
HEADERS += window.hpp function.hpp 3d.hpp msr.hpp threads.hpp
