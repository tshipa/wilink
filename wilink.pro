include(wilink.pri)

TEMPLATE = subdirs

SUBDIRS = src

# Package generation
PACKAGE = wiLink
VERSION = $$WILINK_VERSION
package.tmp = $${PACKAGE}.tmp
mac {
    package.depends = first
    package.output = $$PACKAGE-$$VERSION-mac-10.6.dmg
    package.bundle = $${PACKAGE}.app
    package.commands =  \
        rm -rf $$package.tmp $$package.output; \
        $(MKDIR) $$package.tmp; \
        ln -s /Applications $$package.tmp/Applications; \
        cp -r src/plugins/$$package.bundle $$package.tmp; \
        $$PWD/cmake/copyplugins $$package.tmp/$$package.bundle $$QMAKE_QMAKE \
            $$[QT_INSTALL_PLUGINS]/imageformats/libqgif.* \
            $$[QT_INSTALL_PLUGINS]/imageformats/libqjpeg.* \
            $$[QT_INSTALL_PLUGINS]/sqldrivers/libqsqlite.*; \
        hdiutil create $$package.output -srcdir $$package.tmp -fs HFS+ -volname \"$$PACKAGE $$VERSION\"; \
        rm -rf $$package.tmp
    QMAKE_EXTRA_TARGETS = package
} else:win32 {
    DEL_TREE=rm -rf
    QT_INSTALL_BINS=/usr/i586-mingw32msvc/bin
    QT_INSTALL_PLUGINS=/usr/i586-mingw32msvc/lib/qt4

    bin.files = \
        src/plugins/release/$${PACKAGE}.exe \
        $$QT_INSTALL_BINS/QtCore4.dll \
        $$QT_INSTALL_BINS/QtDeclarative4.dll \
        $$QT_INSTALL_BINS/QtGui4.dll \
        $$QT_INSTALL_BINS/QtMultimedia4.dll \
        $$QT_INSTALL_BINS/QtNetwork4.dll \
        $$QT_INSTALL_BINS/QtScript4.dll \
        $$QT_INSTALL_BINS/QtSql4.dll \
        $$QT_INSTALL_BINS/QtXml4.dll \
        $$QT_INSTALL_BINS/QtXmlPatterns4.dll \
        $$QT_INSTALL_BINS/mingwm10.dll \
        $$QT_INSTALL_BINS/libeay32.dll \
        $$QT_INSTALL_BINS/libgcc_s_dw2-1.dll \
        $$QT_INSTALL_BINS/libogg-0.dll \
        $$QT_INSTALL_BINS/libspeex-1.dll \
        $$QT_INSTALL_BINS/libvorbis-0.dll \
        $$QT_INSTALL_BINS/libvorbisfile-3.dll \
        $$QT_INSTALL_BINS/libtheoradec-1.dll \
        $$QT_INSTALL_BINS/libtheoraenc-1.dll \
        $$QT_INSTALL_BINS/libz.dll \
        $$QT_INSTALL_BINS/ssleay32.dll
    bin.path = $$package.tmp/bin

    imageformats.files = \
        $$QT_INSTALL_PLUGINS/imageformats/qgif4.dll \
        $$QT_INSTALL_PLUGINS/imageformats/qjpeg4.dll
    imageformats.path = $$bin.path/imageformats

    sqldrivers.files = \
        $$QT_INSTALL_PLUGINS/sqldrivers/qsqlite4.dll
    sqldrivers.path = $$bin.path/sqldrivers

    package.depends = first
    package.log = $${PACKAGE}.log
    package.nsi = $${PACKAGE}.nsi
    package.output = $$PACKAGE-$$VERSION-win32.exe
    package.commands = \
        $$DEL_TREE $$package.tmp; \
        $(MKDIR) $$package.tmp; \
        $(MKDIR) $$bin.path; \
        $(COPY_FILE) $$bin.files $$bin.path; \
        $(MKDIR) $$imageformats.path; \
        $(COPY_FILE) $$imageformats.files $$imageformats.path; \
        $(MKDIR) $$sqldrivers.path; \
        $(COPY_FILE) $$sqldrivers.files $$sqldrivers.path; \
        sed -e \"s,@VERSION@,$$VERSION,g\" -e \"s,@INST_DIR@,$$package.tmp,g\" -e \"s,@OUTFILE@,$$package.output,g\" src/data/$${PACKAGE}.nsi > $$package.nsi; \
        makensis -O$$package.log $$package.nsi; \
        $(DEL_FILE) $$package.log $$package.nsi; \
        $$DEL_TREE $$package.tmp
    QMAKE_EXTRA_TARGETS = package
}

OTHER_FILES += \
    android/AndroidManifest.xml \
    android/res/drawable/icon.png \
    android/res/drawable/logo.png \
    android/res/drawable-ldpi/icon.png \
    android/res/drawable-hdpi/icon.png \
    android/res/drawable-mdpi/icon.png \
    android/res/layout/splash.xml \
    android/res/values/libs.xml \
    android/res/values/strings.xml \
    android/res/values-de/strings.xml \
    android/res/values-et/strings.xml \
    android/res/values-ru/strings.xml \
    android/res/values-pt-rBR/strings.xml \
    android/res/values-id/strings.xml \
    android/res/values-nb/strings.xml \
    android/res/values-ja/strings.xml \
    android/res/values-es/strings.xml \
    android/res/values-nl/strings.xml \
    android/res/values-rs/strings.xml \
    android/res/values-ms/strings.xml \
    android/res/values-zh-rCN/strings.xml \
    android/res/values-it/strings.xml \
    android/res/values-ro/strings.xml \
    android/res/values-fa/strings.xml \
    android/res/values-pl/strings.xml \
    android/res/values-zh-rTW/strings.xml \
    android/res/values-el/strings.xml \
    android/res/values-fr/strings.xml \
    android/src/org/kde/necessitas/ministro/IMinistro.aidl \
    android/src/org/kde/necessitas/ministro/IMinistroCallback.aidl \
    android/src/org/kde/necessitas/origo/QtActivity.java \
    android/src/org/kde/necessitas/origo/QtApplication.java \
    android/version.xml

