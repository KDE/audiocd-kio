add_definitions(-DTRANSLATION_DOMAIN=\"audiocd_encoder_vorbis\")

add_library(audiocd_encoder_vorbis SHARED)

target_sources(audiocd_encoder_vorbis PRIVATE
    encodervorbis.cpp
)

ki18n_wrap_ui(audiocd_encoder_vorbis encodervorbisconfig.ui)

kconfig_add_kcfg_files(audiocd_encoder_vorbis audiocd_vorbis_encoder.kcfgc)

target_link_libraries(audiocd_encoder_vorbis 
    OGGVORBIS::vorbis
    audiocdplugins
    KF5::Cddb
    KF5::I18n
    Qt::Widgets
)

install(TARGETS audiocd_encoder_vorbis  DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES audiocd_vorbis_encoder.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR})

