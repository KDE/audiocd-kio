add_definitions(-DTRANSLATION_DOMAIN=\"audiocd_encoder_vorbis\")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..  ${OGGVORBIS_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR})

########### next target ###############

set(audiocd_encoder_vorbis_PART_SRCS encodervorbis.cpp)

ki18n_wrap_ui(audiocd_encoder_vorbis_PART_SRCS encodervorbisconfig.ui)

kconfig_add_kcfg_files(audiocd_encoder_vorbis_PART_SRCS audiocd_vorbis_encoder.kcfgc)

add_library(audiocd_encoder_vorbis ${audiocd_encoder_vorbis_PART_SRCS})
set_target_properties(audiocd_encoder_vorbis PROPERTIES PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}")

target_link_libraries(audiocd_encoder_vorbis 
    ${OGGVORBIS_LIBRARIES} 
    Qt5::Widgets
    KF5::I18n
    KF5::Cddb
    audiocdplugins
)

install(TARGETS audiocd_encoder_vorbis  DESTINATION ${KDE_INSTALL_PLUGINDIR})

########### install files ###############

install(FILES audiocd_vorbis_encoder.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR})

