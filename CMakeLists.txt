#  INDIWebManagerApp
#    Copyright (C) 2019 Robert Lancaster <rlancaste@gmail.com>
#
#    This application is free software; you can redistribute it and/or
#    modify it under the terms of the GNU General Public
#    License as published by the Free Software Foundation; either
#    version 2 of the License, or (at your option) any later version.

######################
project(INDIWebManagerApp)

# Timestamp build
string(TIMESTAMP INDI_WEB_MANAGER_APP_BUILD_TS UTC)

# INDI Web Manager App Version 1.5
set (INDIWebManagerApp_VERSION_MAJOR 1)
set (INDIWebManagerApp_VERSION_MINOR 5)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

cmake_minimum_required (VERSION 2.8.12)
set (CMAKE_CXX_STANDARD 11)



# Fetch translations with -DFETCH_TRANSLATIONS=ON
option(FETCH_TRANSLATIONS "Fetch Translations" OFF)

# Set options for fetching translations
if (FETCH_TRANSLATIONS)
SET(KDE_L10N_AUTO_TRANSLATIONS ON)
SET(KDE_L10N_BRANCH "trunk")
endif()

# This is where the source files are located, including headers
include_directories ("${PROJECT_SOURCE_DIR}/src")
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#This should eliminate the CMP0071 warning
if(POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW)
endif()

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(ECM 1.7.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

find_package(INDI 1.7.1)
if (INDI_FOUND)
#MACRO_BOOL_TO_01(INDI_FOUND HAVE_INDI)
#set_package_properties(INDI PROPERTIES DESCRIPTION "Astronomical instrumentation control" URL "http://www.indilib.org" TYPE OPTIONAL PURPOSE "Support for controlling astronomical devices on Linux with KStars.")
include_directories(${INDI_INCLUDE_DIR})
find_package(Nova REQUIRED)
include_directories(${NOVA_INCLUDE_DIR})
find_package(ZLIB REQUIRED)
endif (INDI_FOUND)

find_package(Qt5 5.5.1 REQUIRED COMPONENTS Widgets Network Sql)

find_package(KF5 REQUIRED COMPONENTS
      Config
      KIO
      DocTools
      I18n
      Notifications
    )

# Are all of these needed?
include(ExternalProject)
include(ECMInstallIcons)
include(ECMAddAppIcon)
include(KDEInstallDirs)
include(MacroBoolTo01)
include(ECMQtDeclareLoggingCategory)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(FeatureSummary)

if (INDI_FOUND)

set(indi_SRCS
    src/indi/drivermanager.cpp
    src/indi/servermanager.cpp
    src/indi/clientmanager.cpp
    src/indi/blobmanager.cpp
    src/indi/guimanager.cpp
    src/indi/driverinfo.cpp
    src/indi/deviceinfo.cpp
    src/indi/indidevice.cpp
    src/indi/indigroup.cpp
    src/indi/indiproperty.cpp
    src/indi/indielement.cpp
    src/indi/indistd.cpp
    src/indi/indilistener.cpp
    src/indi/inditelescope.cpp
    src/indi/indiccd.cpp
    src/indi/indifocuser.cpp
    src/indi/indifilter.cpp
    src/indi/indidome.cpp
    src/indi/indiweather.cpp
    src/indi/indicap.cpp
    src/indi/indilightbox.cpp
    src/indi/indidbus.cpp
    src/indi/customdrivers.cpp
    src/indi/indi_debug.cpp
    src/indi/ksnotification.cpp
)

endif (INDI_FOUND)

set (INDIWebManagerApp_SRCS
    ${indi_SRCS}
    src/main.cpp
    src/mainwindow.cpp
    src/opsconfiguration.cpp
    src/opsmanager.cpp
)

set (INDIWebManagerAppui_SRCS
    src/mainwindow.ui
    src/opsconfiguration.ui
    src/opsmanager.ui
)

ki18n_wrap_ui(INDIWebManagerApp_SRCS ${INDIWebManagerAppui_SRCS})

If(APPLE)
set(MACOSX_BUNDLE_ICON_FILE INDIWebManagerApp.icns)
set(INDIWebManagerApp_ICON icons/INDIWebManagerApp.icns)
set_source_files_properties(${INDIWebManagerApp_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif(APPLE)

ecm_add_app_icon(INDIWebManagerApp_SRCS ICONS
       ${CMAKE_CURRENT_SOURCE_DIR}/icons/128-indi_logo.png
)

qt5_add_resources(INDIWebManagerApp_SRCS resources.qrc)

kconfig_add_kcfg_files(INDIWebManagerApp_SRCS Options.kcfgc)

add_executable(INDIWebManagerApp ${INDIWebManagerApp_ICON} ${INDIWebManagerApp_SRCS})

target_link_libraries(INDIWebManagerApp
        KF5::I18n
        KF5::ConfigCore
        KF5::ConfigGui
        KF5::KIOFileWidgets
        Qt5::Network
        Qt5::Widgets
        Qt5::Sql
        KF5::Notifications
        ${INDI_CLIENT_LIBRARIES}
        ${NOVA_LIBRARIES}
        ${ZLIB_LIBRARIES}
)

install(TARGETS INDIWebManagerApp ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
########### install files ###############
#install(PROGRAMS org.kde.kstars.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES INDIWebManagerApp.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
