cmake_minimum_required(VERSION 3.21)

# ---------------------------------------------------------------------------
# Project metadata + version derived from git tag.
# ---------------------------------------------------------------------------
include(cmake/Version.cmake)
nodetalk_detect_version(NODETALK_VERSION NODETALK_VERSION_FULL)

project(NodeTalk
    VERSION ${NODETALK_VERSION}
    DESCRIPTION "Cross-platform peer-to-peer LAN messenger"
    HOMEPAGE_URL "https://github.com/your-org/node-talk"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(NODETALK_BUILD_TESTS "Build NodeTalk test suite" ON)

# ---------------------------------------------------------------------------
# Qt 6
# ---------------------------------------------------------------------------
find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Sql
    LinguistTools
)
if(NODETALK_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    enable_testing()
endif()

qt_standard_project_setup()

# ---------------------------------------------------------------------------
# Generated headers
# ---------------------------------------------------------------------------
configure_file(
    ${CMAKE_SOURCE_DIR}/src/app/Version.h.in
    ${CMAKE_BINARY_DIR}/generated/app/Version.h
    @ONLY
)

# ---------------------------------------------------------------------------
# Core library (so that tests can link against it)
# ---------------------------------------------------------------------------
set(NODETALK_LIB_SOURCES
    src/app/Paths.cpp
    src/app/Paths.h
    src/app/Logger.cpp
    src/app/Logger.h
    src/core/Identity.cpp
    src/core/Identity.h
    src/core/Settings.cpp
    src/core/Settings.h
    src/core/EventLog.cpp
    src/core/EventLog.h
    src/db/Database.cpp
    src/db/Database.h
    src/db/PeerRepository.cpp
    src/db/PeerRepository.h
    src/db/MessageRepository.cpp
    src/db/MessageRepository.h
    src/db/TransferRepository.cpp
    src/db/TransferRepository.h
    src/db/EventRepository.cpp
    src/db/EventRepository.h
    src/model/Peer.h
    src/model/Message.h
    src/model/Transfer.h
    src/net/Protocol.cpp
    src/net/Protocol.h
    src/net/DiscoveryService.cpp
    src/net/DiscoveryService.h
    src/net/PeerLink.cpp
    src/net/PeerLink.h
    src/net/TransportServer.cpp
    src/net/TransportServer.h
    src/net/PeerManager.cpp
    src/net/PeerManager.h
    src/net/FileTransferManager.cpp
    src/net/FileTransferManager.h
)

qt_add_library(nodetalk_core STATIC ${NODETALK_LIB_SOURCES})

target_include_directories(nodetalk_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(nodetalk_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Sql
)

target_compile_definitions(nodetalk_core PUBLIC
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
    QT_USE_QSTRINGBUILDER
)

if(MSVC)
    target_compile_options(nodetalk_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(nodetalk_core PRIVATE
        -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# ---------------------------------------------------------------------------
# UI sources (kept separate so headless tests can avoid it)
# ---------------------------------------------------------------------------
set(NODETALK_UI_SOURCES
    src/app/Application.cpp
    src/app/Application.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/PeerListModel.cpp
    src/ui/PeerListModel.h
    src/ui/PeerListWidget.cpp
    src/ui/PeerListWidget.h
    src/ui/ChatView.cpp
    src/ui/ChatView.h
    src/ui/MessageInput.cpp
    src/ui/MessageInput.h
    src/ui/TransferWidget.cpp
    src/ui/TransferWidget.h
    src/ui/EventLogWidget.cpp
    src/ui/EventLogWidget.h
    src/ui/TrustDialog.cpp
    src/ui/TrustDialog.h
    src/ui/ManualAddPeerDialog.cpp
    src/ui/ManualAddPeerDialog.h
    src/ui/SettingsDialog.cpp
    src/ui/SettingsDialog.h
    src/ui/TrayIcon.cpp
    src/ui/TrayIcon.h
)

qt_add_library(nodetalk_ui STATIC ${NODETALK_UI_SOURCES})
target_link_libraries(nodetalk_ui PUBLIC nodetalk_core Qt6::Widgets)

# ---------------------------------------------------------------------------
# Translations
# ---------------------------------------------------------------------------
set(NODETALK_TS_FILES
    resources/i18n/nodetalk_en.ts
    resources/i18n/nodetalk_ru.ts
)

qt_add_translations(nodetalk_core
    TS_FILES ${NODETALK_TS_FILES}
    QM_FILES_OUTPUT_VARIABLE NODETALK_QM_FILES
)

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
qt_add_executable(nodetalk
    src/main.cpp
)

# Platform icon assets.
#  * Windows: a .rc references a multi-size .ico and embeds VERSIONINFO.
#  * macOS:   a multi-size .icns is bundled and referenced from Info.plist.
#  * Linux:   PNG hicolor icons are installed below.
if(WIN32)
    # Pass the version into the .rc preprocessor.
    string(REPLACE "." "," NODETALK_VERSION_RC "${PROJECT_VERSION}")
    set(NODETALK_VERSION_RC "${NODETALK_VERSION_RC},0")
    set_source_files_properties(packaging/windows/nodetalk.rc PROPERTIES
        COMPILE_DEFINITIONS
            "NODETALK_VERSION_RC=${NODETALK_VERSION_RC};NODETALK_VERSION_STR=\"${PROJECT_VERSION}\"")
    target_sources(nodetalk PRIVATE packaging/windows/nodetalk.rc)
endif()
if(APPLE)
    set(NODETALK_MACOS_ICON "${CMAKE_SOURCE_DIR}/packaging/macos/nodetalk.icns")
    target_sources(nodetalk PRIVATE "${NODETALK_MACOS_ICON}")
    set_source_files_properties("${NODETALK_MACOS_ICON}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
endif()
target_link_libraries(nodetalk PRIVATE
    nodetalk_core
    nodetalk_ui
    Qt6::Widgets
)

# ---------------------------------------------------------------------------
# Resources (attached to the executable)
# ---------------------------------------------------------------------------
qt_add_resources(nodetalk "app_resources"
    PREFIX "/"
    FILES
        resources/icons/nodetalk.svg
        resources/icons/peer-online.svg
        resources/icons/peer-offline.svg
        resources/icons/send.svg
        resources/icons/attach.svg
)

set_target_properties(nodetalk PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    OUTPUT_NAME "NodeTalk"
)

if(APPLE)
    set_target_properties(nodetalk PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "NodeTalk"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.nodetalk.NodeTalk"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "nodetalk.icns"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/packaging/macos/Info.plist.in"
    )
endif()

# ---------------------------------------------------------------------------
# Install + CPack
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS nodetalk
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
    install(FILES packaging/linux/nodetalk.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
    install(FILES resources/icons/nodetalk.svg
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)
    foreach(_sz 16 24 32 48 64 128 256 512)
        install(FILES "resources/icons/png/nodetalk-${_sz}.png"
            DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/${_sz}x${_sz}/apps"
            RENAME "nodetalk.png")
    endforeach()
endif()

include(cmake/Packaging.cmake)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(NODETALK_BUILD_TESTS)
    add_subdirectory(tests)
endif()

message(STATUS "NodeTalk version: ${NODETALK_VERSION_FULL}")
