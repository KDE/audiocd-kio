
include_directories( ${LIBKCDDB_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/..  ${OGGVORBIS_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR})


########### next target ###############

add_definitions(-DHAVE_VORBIS)

set(audiocd_encoder_vorbis_PART_SRCS encodervorbis.cpp )


kde4_add_ui_files(audiocd_encoder_vorbis_PART_SRCS encodervorbisconfig.ui )

kde4_add_kcfg_files(audiocd_encoder_vorbis_PART_SRCS audiocd_vorbis_encoder.kcfgc )

kde4_add_plugin(audiocd_encoder_vorbis WITH_PREFIX ${audiocd_encoder_vorbis_PART_SRCS})



target_link_libraries(audiocd_encoder_vorbis ${OGGVORBIS_LIBRARIES} ${KDE4_KIO_LIBS} ${LIBKCDDB_LIBRARIES} audiocdplugins)

install(TARGETS audiocd_encoder_vorbis  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES audiocd_vorbis_encoder.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )

