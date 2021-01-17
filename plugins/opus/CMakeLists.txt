add_definitions(-DTRANSLATION_DOMAIN=\"audiocd_encoder_opus\")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

########### next target ###############

set(audiocd_encoder_opus_PART_SRCS encoderopus.cpp)

ki18n_wrap_ui(audiocd_encoder_opus_PART_SRCS encoderopusconfig.ui)

kconfig_add_kcfg_files(audiocd_encoder_opus_PART_SRCS audiocd_opus_encoder.kcfgc)

add_library(audiocd_encoder_opus ${audiocd_encoder_opus_PART_SRCS})
set_target_properties(audiocd_encoder_opus PROPERTIES PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}")

target_link_libraries(audiocd_encoder_opus
    Qt5::Widgets
    KF5::I18n
    KF5::Cddb
    audiocdplugins
)

install(TARGETS audiocd_encoder_opus  DESTINATION ${KDE_INSTALL_PLUGINDIR})

########### install files ###############

install(FILES audiocd_opus_encoder.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR})
