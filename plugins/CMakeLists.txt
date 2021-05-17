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

add_library(audiocdplugins SHARED)

target_sources(audiocdplugins PRIVATE
    audiocdencoder.cpp
)
target_include_directories(audiocdplugins
    PRIVATE
        ${CMAKE_BINARY_DIR}
)
target_link_libraries(audiocdplugins
    PUBLIC
        KF5::KIOCore
        KF5::Cddb
)

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
    audiocdencoder.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}
    COMPONENT Devel
)

