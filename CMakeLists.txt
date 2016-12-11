project(audiocd)

cmake_minimum_required(VERSION 2.8.12)

# ECM setup
include(FeatureSummary)
find_package(ECM 5.24.0  NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://projects.kde.org/projects/kdesupport/extra-cmake-modules")
set(CMAKE_MODULE_PATH "${ECM_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(PROJECT_VERSION "16.07.0")
set(KF5_DEP_VERSION "5.24.0") # handled by release scripts

find_package(KF5 ${KF5_DEP_VERSION} REQUIRED COMPONENTS
    Config
    KDELibs4Support
    I18n
    DocTools
    KIO
    CompactDisc
)
find_package(KF5Cddb)
set_package_properties(KF5Cddb PROPERTIES
    DESCRIPTION "KF5 branch for CDDB library"
    URL "http://projects.kde.org/projects/kde/kdemultimedia/libkcddb"
    TYPE REQUIRED
    PURPOSE "libkcddb is used to retrieve audio CD meta data from the internet."
)

include(KDEInstallDirs)
include(KDECompilerSettings)
include(KDECMakeSettings)
include(GenerateExportHeader)
include(ECMSetupVersion)
include(CMakePushCheckState)
include(CheckStructHasMember)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX AUDIOCDPLUGINS
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/audiocdplugins_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/AudioCDPluginsConfigVersion.cmake"
    SOVERSION 16
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

add_library(kio_audiocd ${kio_audiocd_PART_SRCS})

target_link_libraries(kio_audiocd 
    ${CDPARANOIA_LIBRARIES} 
    KF5::CompactDisc
    audiocdplugins
)

install(TARGETS kio_audiocd  DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install(FILES audiocd.protocol  DESTINATION  ${SERVICES_INSTALL_DIR})

