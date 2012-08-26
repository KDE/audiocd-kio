
include_directories( ${LIBKCDDB_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/..  )

FIND_PROGRAM(LAME_EXECUTABLE NAMES lame
)

if(NOT LAME_EXECUTABLE)
	MESSAGE(STATUS "\"lame\" binary not found. Plugins will be compile but install it under your system to using this plugins")
endif(NOT LAME_EXECUTABLE)

########### next target ###############

set(audiocd_encoder_lame_PART_SRCS encoderlame.cpp )


kde4_add_ui_files(audiocd_encoder_lame_PART_SRCS encoderlameconfig.ui )

kde4_add_kcfg_files(audiocd_encoder_lame_PART_SRCS audiocd_lame_encoder.kcfgc )

kde4_add_plugin(audiocd_encoder_lame WITH_PREFIX ${audiocd_encoder_lame_PART_SRCS})



target_link_libraries(audiocd_encoder_lame  ${KDE4_KIO_LIBS} ${LIBKCDDB_LIBRARIES} audiocdplugins)

install(TARGETS audiocd_encoder_lame  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES audiocd_lame_encoder.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )

