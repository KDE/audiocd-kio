find_program(LAME_EXECUTABLE NAMES lame)

if(NOT LAME_EXECUTABLE)
    message(STATUS "\"lame\" binary not found. Plugins will be compile but install it under your system to using this plugins")
endif()


add_definitions(-DTRANSLATION_DOMAIN=\"audiocd_encoder_lame\")

add_library(audiocd_encoder_lame SHARED)

target_sources(audiocd_encoder_lame PRIVATE
    encoderlame.cpp
)

ki18n_wrap_ui(audiocd_encoder_lame encoderlameconfig.ui)

kconfig_add_kcfg_files(audiocd_encoder_lame audiocd_lame_encoder.kcfgc)

target_include_directories(audiocd_encoder_lame
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(audiocd_encoder_lame 
    audiocdplugins
    KF5::Cddb
    KF5::I18n
    Qt::Widgets
)

install(TARGETS audiocd_encoder_lame  DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES audiocd_lame_encoder.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR})

