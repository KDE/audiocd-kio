add_definitions(-DTRANSLATION_DOMAIN=\"audiocd_encoder_lame\")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

FIND_PROGRAM(LAME_EXECUTABLE NAMES lame)

if(NOT LAME_EXECUTABLE)
	MESSAGE(STATUS "\"lame\" binary not found. Plugins will be compile but install it under your system to using this plugins")
endif(NOT LAME_EXECUTABLE)

########### next target ###############

set(audiocd_encoder_lame_PART_SRCS encoderlame.cpp )

ki18n_wrap_ui(audiocd_encoder_lame_PART_SRCS encoderlameconfig.ui)

kconfig_add_kcfg_files(audiocd_encoder_lame_PART_SRCS audiocd_lame_encoder.kcfgc)

add_library(audiocd_encoder_lame ${audiocd_encoder_lame_PART_SRCS})
set_target_properties(audiocd_encoder_lame PROPERTIES PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}")

target_link_libraries(audiocd_encoder_lame 
    KF5::I18n
    KF5::Cddb
    audiocdplugins
)

install(TARGETS audiocd_encoder_lame  DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install(FILES audiocd_lame_encoder.kcfg  DESTINATION  ${KCFG_INSTALL_DIR})

