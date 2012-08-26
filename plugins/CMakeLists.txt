
macro_optional_find_package(Flac)
macro_log_feature(FLAC_FOUND "FLAC" "An encoder/decoder for the Free Lossless Audio Codec" "http://flac.sourceforge.net" FALSE "" "Required to build the AudioCD FLAC plugin.")

if (FLAC_FOUND)
    add_subdirectory( flac )
endif(FLAC_FOUND)

macro_optional_find_package(OggVorbis)
macro_log_feature(OGGVORBIS_FOUND "OggVorbis" "Libraries for Ogg Vorbis general audio compression codec" "http://www.xiph.org" FALSE "" "Required for the AudioCD Ogg Vorbis plugin.")

if (OGGVORBIS_FOUND)
    add_subdirectory( vorbis )
endif(OGGVORBIS_FOUND)

add_subdirectory( wav )
add_subdirectory( lame )

include_directories( ${LIBKCDDB_INCLUDE_DIRS}  )


########### next target ###############

set(audiocdplugins_LIB_SRCS audiocdencoder.cpp )


kde4_add_library(audiocdplugins SHARED ${audiocdplugins_LIB_SRCS})

target_link_libraries(audiocdplugins  ${KDE4_KDECORE_LIBS} ${LIBKCDDB_LIBRARIES} )

set_target_properties(audiocdplugins PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS audiocdplugins ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( FILES audiocdencoder.h  DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel)

