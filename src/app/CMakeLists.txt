set(QT_USE_QTDECLARATIVE 1)
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTMULTIMEDIA 1)
set(QT_USE_QTSQL 1)
set(QT_USE_QTWEBKIT 1)
set(QT_USE_QTXML 1)
include(${QT_USE_FILE})

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../qdjango/src/db)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../qnetio/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../qsound/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../qxmpp/src/base)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../qxmpp/src/client)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../qxmpp-extra/diagnostics)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../qxmpp-extra/shares)

# Libraries
set(wilink_LIBRARIES ${QT_QTGUI_LIBRARY} ${QT_QTDECLARATIVE_LIBRARY} ${QT_QTWEBKIT_LIBRARY})

# Needed for Qt >= 4.7 on Debian/Ubuntu
find_library(QTMULTIMEDIAKIT_LIBRARY QtMultimediaKit)
if(QTMULTIMEDIAKIT_LIBRARY)
    message(STATUS "Found QtMultimediaKit library")
    include_directories(/usr/include/QtMobility)
    include_directories(/usr/include/QtMultimediaKit)
endif()

if (APPLE)
    list(APPEND wilink_LIBRARIES "-framework AppKit")

    # Growl support
    find_library(GROWL_LIBRARY Growl)
    if(GROWL_LIBRARY)
        message(STATUS "Found Growl library")
        add_definitions(-DUSE_GROWL)
        list(APPEND wilink_LIBRARIES ${GROWL_LIBRARY})
    endif()
elseif (UNIX)
    # libnotify support
    find_package(PkgConfig)
    pkg_check_modules(LIBNOTIFY "libnotify")
    if(LIBNOTIFY_FOUND)
        message(STATUS "Found libnotify library")
        include_directories(${LIBNOTIFY_INCLUDE_DIRS})
        add_definitions(-DUSE_LIBNOTIFY)
        list(APPEND wilink_LIBRARIES ${LIBNOTIFY_LIBRARIES})
    endif()
endif()

# wiLink
add_definitions(-DWILINK_VERSION="${WILINK_VERSION}")
if(WILINK_EMBEDDED)
    add_definitions(-DWILINK_EMBEDDED)
endif()
add_definitions(-DQT_STATICPLUGIN)
if(NOT APPLE)
    add_definitions(-DUSE_SYSTRAY)
endif(NOT APPLE)

set(wiLink_SOURCES
    accounts.cpp
    application.cpp
    calls.cpp
    client.cpp
    console.cpp
    conversations.cpp
    declarative.cpp
    diagnostics.cpp
    discovery.cpp
    history.cpp
    main.cpp
    model.cpp
    news.cpp
    photos.cpp
    phone.cpp
    phone/sip.cpp
    phone/stun.cpp
    player.cpp
    rooms.cpp
    roster.cpp
    shares.cpp
    systeminfo.cpp
    updater.cpp
    window.cpp)

if(APPLE)
    set(wiLink_SOURCES ${wiLink_SOURCES} application_mac.mm)
endif(APPLE)

set(wiLink_MOC_HEADERS
    application.h
    accounts.h
    calls.h
    client.h
    console.h
    conversations.h
    declarative.h
    diagnostics.h
    discovery.h
    history.h
    model.h
    news.h
    photos.h
    phone.h
    phone/sip.h
    phone/stun.h
    player.h
    rooms.h
    roster.h
    shares.h
    updater.h
    window.h)

configure_file(../translations/translations.qrc.cmake ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)
if(RELOAD_TRANSLATIONS)
    qt4_create_translation(QM_FILES
        application.cpp
        window.cpp
        ../data/AboutDialog.qml
        ../data/AccountAddPanel.qml
        ../data/AccountListPanel.qml
        ../data/AccountPasswordDialog.qml
        ../data/AccountPreferencePanel.qml
        ../data/CallNotification.qml
        ../data/CallWidget.qml
        ../data/ChatEdit.qml
        ../data/ChatPanel.qml
        ../data/ChatPlugin.qml
        ../data/ContactAddDialog.qml
        ../data/ContactAddNotification.qml
        ../data/ContactRenameDialog.qml
        ../data/ContactRemoveDialog.qml
        ../data/ConversationPanel.qml
        ../data/DebugPlugin.qml
        ../data/DiagnosticPanel.qml
        ../data/DiagnosticPlugin.qml
        ../data/Dialog.qml
        ../data/DiscoveryPanel.qml
        ../data/Dock.qml
        ../data/ErrorNotification.qml
        ../data/GeneralPreferencePanel.qml
        ../data/HistoryView.qml
        ../data/InputMenu.qml
        ../data/LogPanel.qml
        ../data/MainWindow.qml
        ../data/NewsDialog.qml
        ../data/NewsPanel.qml
        ../data/NewsPlugin.qml
        ../data/PhoneContactDialog.qml
        ../data/PhoneContactView.qml
        ../data/PhoneHistoryView.qml
        ../data/PhoneNotification.qml
        ../data/PhonePanel.qml
        ../data/PhonePlugin.qml
        ../data/PhotoAlbumDialog.qml
        ../data/PhotoDeleteDialog.qml
        ../data/PhotoPanel.qml
        ../data/PhotoPlugin.qml
        ../data/PlayerPanel.qml
        ../data/PlayerPlugin.qml
        ../data/PluginPreferencePanel.qml
        ../data/PreferenceDialog.qml
        ../data/PreferenceModel.qml
        ../data/RoomInviteDialog.qml
        ../data/RoomInviteNotification.qml
        ../data/RoomJoinDialog.qml
        ../data/RoomKickDialog.qml
        ../data/RoomPanel.qml
        ../data/RoomParticipantView.qml
        ../data/RoomPermissionDialog.qml
        ../data/RoomSubjectDialog.qml
        ../data/SetupWindow.qml
        ../data/SharePanel.qml
        ../data/SharePlugin.qml
        ../data/SharePreferencePanel.qml
        ../data/ShareQueueView.qml
        ../data/ShareView.qml
        ../data/SoundPreferencePanel.qml
        ../data/StatusBar.qml
        ../data/TransferNotification.qml
        ../data/TransferWidget.qml
        ../data/WifirstPlugin.qml
        ../translations/fr.ts)
else(RELOAD_TRANSLATIONS)
    qt4_add_translation(QM_FILES ../translations/fr.ts)
endif(RELOAD_TRANSLATIONS)
qt4_add_translation(QM_FILES ../translations/qt_fr.ts)

set(wiLink_RCCS
    ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/wiLink.qrc)
if(WILINK_EMBEDDED)
    list(APPEND wiLink_RCCS
        ${CMAKE_CURRENT_SOURCE_DIR}/../data/32x32/tiny.qrc
        ${CMAKE_CURRENT_SOURCE_DIR}/../data/64x64/normal.qrc)
else()
    list(APPEND wiLink_RCCS
        ${CMAKE_CURRENT_SOURCE_DIR}/../data/16x16/tiny.qrc
        ${CMAKE_CURRENT_SOURCE_DIR}/../data/32x32/normal.qrc)
endif()

qt4_add_resources(wiLink_RCC_SOURCES ${wiLink_RCCS})
qt4_wrap_cpp(wiLink_MOC_SOURCES ${wiLink_MOC_HEADERS})

if(WIN32)
    enable_language(RC)
    set(WILINK_VERSION_WIN32 "${WILINK_VERSION_MAJOR},${WILINK_VERSION_MINOR},${WILINK_VERSION_PATCH},0")
    configure_file(${CMAKE_SOURCE_DIR}/cmake/wiLink.rc.in ${CMAKE_CURRENT_BINARY_DIR}/wiLink.rc)
    set(WILINK_RESOURCE wiLink.rc)
endif(WIN32)

add_executable(wiLink ${wiLink_SOURCES} ${wiLink_MOC_SOURCES} ${wiLink_RCC_SOURCES} ${WILINK_RESOURCE})
target_link_libraries(wiLink idle qnetio qxmpp qxmpp-diagnostics qxmpp-shares qdjango-db qsound ${wilink_LIBRARIES})

if(APPLE)
    get_filename_component(BUNDLE_ICON_NAME ${CPACK_BUNDLE_ICON} NAME_WE)
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE TRUE)
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER ${CPACK_BUNDLE_IDENTIFIER})
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_ICON_FILE ${BUNDLE_ICON_NAME})
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME ${CPACK_BUNDLE_NAME})
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION ${WILINK_VERSION})
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_LONG_VERSION_STRING ${WILINK_VERSION})
    set_target_properties(wiLink PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING ${WILINK_VERSION})
    add_custom_command(TARGET wiLink POST_BUILD
        COMMAND mkdir -p ${BUNDLE_LOCATION}/Contents/Resources/
        COMMAND touch ${BUNDLE_LOCATION}/Contents/Resources/empty.lproj
        COMMAND cp ${CPACK_BUNDLE_ICON} ${BUNDLE_LOCATION}/Contents/Resources/
        COMMAND ${CMAKE_SOURCE_DIR}/cmake/copyplugins -b ${BUNDLE_LOCATION} -q ${QT_QMAKE_EXECUTABLE} -f "${WILINK_EXTRA_FRAMEWORKS}" -i "${WILINK_EXTRA_IMPORTS}" ${WILINK_EXTRA_PLUGINS}
        DEPENDS wiLink)
endif(APPLE)
if(WIN32)
    set_target_properties(wiLink PROPERTIES LINK_FLAGS "-mwindows")
endif(WIN32)

install(TARGETS wiLink DESTINATION bin)

# tests
#add_subdirectory(tests)