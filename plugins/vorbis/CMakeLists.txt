
include_directories( ${CMAKE_SOURCE_DIR}/libkcddb ${CMAKE_CURRENT_SOURCE_DIR}/..  ${OGGVORBIS_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR})


########### next target ###############

set(audiocd_encoder_vorbis_PART_SRCS encodervorbis.cpp )


kde4_add_ui_files(audiocd_encoder_vorbis_PART_SRCS encodervorbisconfig.ui )

kde4_add_kcfg_files(audiocd_encoder_vorbis_PART_SRCS audiocd_vorbis_encoder.kcfgc )

kde4_add_plugin(audiocd_encoder_vorbis WITH_PREFIX ${audiocd_encoder_vorbis_PART_SRCS})



target_link_libraries(audiocd_encoder_vorbis ${OGGVORBIS_LIBRARIES} ${KDE4_KIO_LIBS} audiocdplugins ${QT_QT3SUPPORT_LIBRARY} kcddb)

install(TARGETS audiocd_encoder_vorbis  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES audiocd_vorbis_encoder.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )

