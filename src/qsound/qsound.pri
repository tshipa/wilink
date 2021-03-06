# Common definitions

QSOUND_INCLUDE_DIR = $$PWD/src
QSOUND_LIBRARY_NAME = qsound

android {
    QSOUND_INCLUDE_DIR += $$PWD/src/fake
} else:mac {
    QT += multimedia
} else:win32 {
    QT += multimedia
} else {
    CONFIG += mobility
    MOBILITY += multimedia
}

# Libraries used internally by QSound
QSOUND_INTERNAL_DEFINES += QSOUND_USE_MAD
QSOUND_INCLUDE_DIR += $$PWD/../3rdparty/libmad
QSOUND_INTERNAL_LIBS += -lmad

mac {
    # qtkit support
    QSOUND_INTERNAL_LIBS += -framework QTKit -framework QuartzCore -framework Cocoa
} else:win32 {
    # directshow support
    QSOUND_INTERNAL_LIBS += -lole32 -loleaut32 -lwinmm -luuid
}

# Libraries for apps which use QSound
symbian {
    # Symbian needs a .lib extension to recognise the library as static
    QSOUND_LIBS = -l$${QSOUND_LIBRARY_NAME}.lib
} else {
    QSOUND_LIBS = -l$${QSOUND_LIBRARY_NAME}
}

# FIXME: we should be able to use the link_prl option to automatically pull in
# the extra libraries which our library needs, but this does not seem to work.
QSOUND_LIBS += $$QSOUND_INTERNAL_LIBS
