
if (FLAC_FOUND)
    add_subdirectory( flac ) 
endif(FLAC_FOUND)

if (OGGVORBIS_FOUND)
    add_subdirectory( vorbis ) 
endif(OGGVORBIS_FOUND)

add_subdirectory( wav ) 
add_subdirectory( lame ) 

include_directories( ${CMAKE_SOURCE_DIR}/libkcddb  )


########### next target ###############

set(audiocdplugins_LIB_SRCS audiocdencoder.cpp )

kde4_automoc(audiocdplugins ${audiocdplugins_LIB_SRCS})

kde4_add_library(audiocdplugins SHARED ${audiocdplugins_LIB_SRCS})

target_link_libraries(audiocdplugins  ${KDE4_KDECORE_LIBS} kcddb )

set_target_properties(audiocdplugins PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS audiocdplugins  DESTINATION ${LIB_INSTALL_DIR} )


########### install files ###############

install( FILES audiocdencoder.h  DESTINATION ${INCLUDE_INSTALL_DIR} )

