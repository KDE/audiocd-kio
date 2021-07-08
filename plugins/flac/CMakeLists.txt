add_definitions(-DTRANSLATION_DOMAIN=\"audiocd_encoder_flac\")

add_library(audiocd_encoder_flac SHARED)

target_sources(audiocd_encoder_flac PRIVATE
    encoderflac.cpp
)

ki18n_wrap_ui(audiocd_encoder_flac encoderflacconfig.ui)

kconfig_add_kcfg_files(audiocd_encoder_flac audiocd_flac_encoder.kcfgc)

target_include_directories(audiocd_encoder_flac
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(audiocd_encoder_flac 
    audiocdplugins
    KF5::I18n
    Qt::Widgets
    FLAC::FLAC
)

install(TARGETS audiocd_encoder_flac  DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES audiocd_flac_encoder.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})

