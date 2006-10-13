
include_directories( ${CMAKE_SOURCE_DIR}/libkcddb ${CMAKE_CURRENT_SOURCE_DIR}/..  )


########### next target ###############

set(audiocd_encoder_lame_PART_SRCS encoderlame.cpp collectingprocess.cpp )

kde4_automoc(audiocd_encoder_lame ${audiocd_encoder_lame_PART_SRCS})

kde4_add_ui3_files(audiocd_encoder_lame_PART_SRCS encoderlameconfig.ui )

kde4_add_kcfg_files(audiocd_encoder_lame audiocd_encoder_lame_PART_SRCS audiocd_lame_encoder.kcfgc )

kde4_add_plugin(audiocd_encoder_lame WITH_PREFIX ${audiocd_encoder_lame_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} audiocd_encoder_lame )

target_link_libraries(audiocd_encoder_lame  ${KDE4_KIO_LIBS} audiocdplugins )

install(TARGETS audiocd_encoder_lame  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES audiocd_lame_encoder.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )

