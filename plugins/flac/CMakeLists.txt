
include_directories( ${LIBKCDDB_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/..  ${FLAC_INCLUDE_DIR} )


########### next target ###############

# Remove this, and #ifdef in encoderflac when Makefile.am's are removed
add_definitions(-DHAVE_LIBFLAC)

set(audiocd_encoder_flac_PART_SRCS encoderflac.cpp )


kde4_add_plugin(audiocd_encoder_flac WITH_PREFIX ${audiocd_encoder_flac_PART_SRCS})



target_link_libraries(audiocd_encoder_flac ${FLAC_LIBRARIES} ${KDE4_KIO_LIBS} ${LIBKCDDB_LIBRARIES} audiocdplugins)

install(TARGETS audiocd_encoder_flac  DESTINATION ${PLUGIN_INSTALL_DIR} )

