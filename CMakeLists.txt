project(audiocd)

cmake_minimum_required(VERSION 2.8.12)

# ECM setup
include(FeatureSummary)
find_package(ECM 5.24.0  NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://projects.kde.org/projects/kdesupport/extra-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)
set(CMAKE_MODULE_PATH "${ECM_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(KF5_VERSION "5.25.0") # handled by release scripts
set(KF5_DEP_VERSION "5.24.0") # handled by release scripts

find_package(KF5 ${KF5_DEP_VERSION} REQUIRED COMPONENTS
    Config
    KDELibs4Support
    DocTools
    KIO
    CompactDisc
    Cddb
)

include(KDEInstallDirs)
include(KDECompilerSettings)
include(KDECMakeSettings)
include(GenerateExportHeader)
include(ECMSetupVersion)

ecm_setup_version(${KF5_VERSION}
    VARIABLE_PREFIX AUDIOCDPLUGINS
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/audiocdplugins_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/AudioCDPluginsConfigVersion.cmake"
    SOVERSION 5
)

find_package(Cdparanoia REQUIRED)

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

