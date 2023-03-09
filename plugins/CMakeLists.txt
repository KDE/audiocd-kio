find_package(FLAC)
if(FLAC_FOUND)
    add_subdirectory(flac)
endif(FLAC_FOUND)

find_package(OggVorbis)
if(OGGVORBIS_FOUND AND HAVE_LIBVORBISENC2)
    add_subdirectory(vorbis)
endif(OGGVORBIS_FOUND AND HAVE_LIBVORBISENC2)

add_subdirectory(wav)
add_subdirectory(lame)
add_subdirectory(opus)

########### next target ###############

set(AUDIOCDPLUGINS_INSTALL_INCLUDEDIR "${KDE_INSTALL_INCLUDEDIR}/audiocdplugins")

ecm_setup_version(PROJECT
    VARIABLE_PREFIX AUDIOCDPLUGINS
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/audiocdplugins_version.h"
    SOVERSION 5
)

add_library(audiocdplugins SHARED)

target_sources(audiocdplugins PRIVATE
    audiocdencoder.cpp audiocdencoder.h
)
target_include_directories(audiocdplugins
    PRIVATE
        ${CMAKE_BINARY_DIR}
)
target_link_libraries(audiocdplugins
    PUBLIC
        KF${KF_MAJOR_VERSION}::KIOCore
)
if (QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(audiocdplugins PUBLIC KCddb6)
else()
    target_link_libraries(audiocdplugins PUBLIC KF5::Cddb)
endif()

generate_export_header(audiocdplugins 
    EXPORT_MACRO_NAME AUDIOCDPLUGINS_EXPORT
    DEPRECATED_MACRO_NAME AUDIOCDPLUGINS_EXPORT_DEPRECATED
    EXPORT_FILE_NAME audiocdplugins_export.h
)

set_target_properties(audiocdplugins PROPERTIES
    VERSION ${AUDIOCDPLUGINS_VERSION}
    SOVERSION ${AUDIOCDPLUGINS_SOVERSION}
)

install(TARGETS audiocdplugins ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/audiocdplugins_export.h
    ${CMAKE_CURRENT_BINARY_DIR}/audiocdplugins_version.h
    audiocdencoder.h
    DESTINATION ${AUDIOCDPLUGINS_INSTALL_INCLUDEDIR}
    COMPONENT Devel
)

