add_definitions(-DTRANSLATION_DOMAIN=\"audiocd_encoder_opus\")

add_library(audiocd_encoder_opus SHARED)

target_sources(audiocd_encoder_opus PRIVATE
    encoderopus.cpp
)

ki18n_wrap_ui(audiocd_encoder_opus encoderopusconfig.ui)

kconfig_add_kcfg_files(audiocd_encoder_opus audiocd_opus_encoder.kcfgc)

target_include_directories(audiocd_encoder_opus
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(audiocd_encoder_opus
    audiocdplugins
    KF5::Cddb
    KF5::I18n
    Qt::Widgets
)

install(TARGETS audiocd_encoder_opus  DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES audiocd_opus_encoder.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR})
