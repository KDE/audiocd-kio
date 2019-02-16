cmake_minimum_required(VERSION 3.0)

project(audiocd)

set(PROJECT_VERSION "19.04.0")

set(QT_MIN_VERSION "5.9.0")
set(KF_MIN_VERSION "5.24.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMSetupVersion)
include(ECMQtDeclareLoggingCategory)
include(CMakePushCheckState)
include(GenerateExportHeader)
include(CheckStructHasMember)
include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Widgets)

find_package(KF5 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Config
    I18n
    DocTools
    KIO
    KCMUtils
)
find_package(KF5Cddb)
set_package_properties(KF5Cddb PROPERTIES
    DESCRIPTION "KF5 branch for CDDB library"
    URL "http://projects.kde.org/projects/kde/kdemultimedia/libkcddb"
    TYPE REQUIRED
    PURPOSE "libkcddb is used to retrieve audio CD meta data from the internet."
)
find_package(KF5CompactDisc)
set_package_properties(KF5CompactDisc PROPERTIES
    DESCRIPTION "KCompactDisc library"
    URL "https://commits.kde.org/libkcompactdisc"
    TYPE REQUIRED
    PURPOSE "libkcompactdisc is used to access CD drives."
)

ecm_setup_version("5.0.0"
    VARIABLE_PREFIX AUDIOCDPLUGINS
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/audiocdplugins_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/AudioCDPluginsConfigVersion.cmake"
    SOVERSION 5
)

find_package(Cdparanoia REQUIRED)

cmake_push_check_state()
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${CDPARANOIA_INCLUDE_DIR})
check_struct_has_member("struct cdrom_drive" "ioctl_device_name" "cdda_interface.h" CDDA_IOCTL_DEVICE_EXISTS)
cmake_pop_check_state()
if(CDDA_IOCTL_DEVICE_EXISTS)
    set(HAVE_CDDA_IOCTL_DEVICE 1)
else()
    set(HAVE_CDDA_IOCTL_DEVICE 0)
endif()
execute_process(
    COMMAND sed -e "s|cdda_private_data_t \\*private;|cdda_private_data_t \\*private_data;|g"
    # The next line should be removed if strerror_tr will be used somewhere
    COMMAND sed -e "/static\ char/,/}\;/d"
    INPUT_FILE ${CDPARANOIA_INCLUDE_DIR}/cdda_interface.h
    OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/cdda_interface.hpp
    RESULT_VARIABLE CDDA_INTERFACE_EDIT_ERROR
)
if (CDDA_INTERFACE_EDIT_ERROR)
    message(STATUS "Fixing cdda_interface.h for C++ failed with exit code ${CDDA_INTERFACE_EDIT_ERROR}")
endif()

configure_file(config-audiocd.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-audiocd.h)

#############################

include_directories(
	${CDPARANOIA_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory(plugins)
add_subdirectory(kcmaudiocd)
add_subdirectory(data)
add_subdirectory(doc)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

########### next target ###############

set(kio_audiocd_PART_SRCS audiocd.cpp )

ecm_qt_declare_logging_category(kio_audiocd_PART_SRCS HEADER audiocd_kio_debug.h IDENTIFIER AUDIOCD_KIO_LOG CATEGORY_NAME kf5.kio.audiocd DEFAULT_SEVERITY Warning)

add_library(kio_audiocd MODULE ${kio_audiocd_PART_SRCS})
set_target_properties(kio_audiocd PROPERTIES OUTPUT_NAME "audiocd")

target_link_libraries(kio_audiocd 
    ${CDPARANOIA_LIBRARIES}
    Qt5::Widgets
    KF5::CompactDisc
    audiocdplugins
)

install(TARGETS kio_audiocd  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)

########### install files ###############

install(FILES kio_audiocd.categories DESTINATION ${KDE_INSTALL_CONFDIR})
install(FILES audiocd.protocol  DESTINATION  ${SERVICES_INSTALL_DIR})

