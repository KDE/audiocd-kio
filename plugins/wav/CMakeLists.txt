
include_directories( ${LIBKCDDB_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/..  ${CDPARANOIA_INCLUDE_DIR} )


########### next target ###############

set(audiocd_encoder_wav_PART_SRCS encoderwav.cpp encodercda.cpp )


kde4_add_plugin(audiocd_encoder_wav WITH_PREFIX ${audiocd_encoder_wav_PART_SRCS})



target_link_libraries(audiocd_encoder_wav  ${KDE4_KIO_LIBS} audiocdplugins )

install(TARGETS audiocd_encoder_wav  DESTINATION ${PLUGIN_INSTALL_DIR} )

