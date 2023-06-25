add_definitions(-DTRANSLATION_DOMAIN=\"audiocd_encoder_opus\")

add_library(audiocd_encoder_opus SHARED)

target_sources(audiocd_encoder_opus PRIVATE
    encoderopus.cpp encoderopus.h
)

ki18n_wrap_ui(audiocd_encoder_opus encoderopusconfig.ui)

kconfig_add_kcfg_files(audiocd_encoder_opus audiocd_opus_encoder.kcfgc)

target_include_directories(audiocd_encoder_opus
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(audiocd_encoder_opus
    audiocdplugins
    KF${KF_MAJOR_VERSION}::I18n
    Qt::Widgets
)
if (QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(audiocd_encoder_opus KCddb6)
else()
    target_link_libraries(audiocd_encoder_opus KF5::Cddb)
endif()

install(TARGETS audiocd_encoder_opus  DESTINATION ${KDE_INSTALL_PLUGINDIR}/audiocd)

install(FILES audiocd_opus_encoder.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR})
