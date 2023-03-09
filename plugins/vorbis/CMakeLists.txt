add_definitions(-DTRANSLATION_DOMAIN=\"audiocd_encoder_vorbis\")

add_library(audiocd_encoder_vorbis SHARED)

target_sources(audiocd_encoder_vorbis PRIVATE
    encodervorbis.cpp encodervorbis.h
)

ki18n_wrap_ui(audiocd_encoder_vorbis encodervorbisconfig.ui)

kconfig_add_kcfg_files(audiocd_encoder_vorbis audiocd_vorbis_encoder.kcfgc)

target_link_libraries(audiocd_encoder_vorbis 
    OGGVORBIS::vorbis
    audiocdplugins
    KF${KF_MAJOR_VERSION}::I18n
    Qt::Widgets
)
if (QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(audiocd_encoder_vorbis KCddb6)
else()
    target_link_libraries(audiocd_encoder_vorbis KF5::Cddb)
endif()

install(TARGETS audiocd_encoder_vorbis  DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES audiocd_vorbis_encoder.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR})

