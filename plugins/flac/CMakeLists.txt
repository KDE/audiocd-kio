add_definitions(-DTRANSLATION_DOMAIN=\"audiocd_encoder_flac\")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..  ${FLAC_INCLUDE_DIR} )

########### next target ###############

# Remove this, and #ifdef in encoderflac when Makefile.am's are removed
add_definitions(-DHAVE_LIBFLAC)

set(audiocd_encoder_flac_PART_SRCS encoderflac.cpp )

ki18n_wrap_ui(audiocd_encoder_flac_PART_SRCS encoderflacconfig.ui)

kconfig_add_kcfg_files(audiocd_encoder_flac_PART_SRCS audiocd_flac_encoder.kcfgc)

add_library(audiocd_encoder_flac ${audiocd_encoder_flac_PART_SRCS})
set_target_properties(audiocd_encoder_flac PROPERTIES PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}")

target_link_libraries(audiocd_encoder_flac 
    ${FLAC_LIBRARIES}
    Qt5::Widgets
    KF5::I18n
    KF5::Cddb
    audiocdplugins
)

install(TARGETS audiocd_encoder_flac  DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES audiocd_flac_encoder.kcfg DESTINATION ${KCFG_INSTALL_DIR})

