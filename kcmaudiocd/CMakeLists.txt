add_definitions(-DTRANSLATION_DOMAIN=\"kcmaudiocd\")

add_library(kcm_audiocd MODULE)

target_sources(kcm_audiocd PRIVATE
    kcmaudiocd.cpp
)

ki18n_wrap_ui(kcm_audiocd audiocdconfig.ui)

target_include_directories(kcm_audiocd
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(kcm_audiocd
    audiocdplugins 
    KF5::KCMUtils
    KF5::I18n
)

install(TARGETS kcm_audiocd  DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES audiocd.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR})

########### add handbook ################

add_subdirectory(doc)
