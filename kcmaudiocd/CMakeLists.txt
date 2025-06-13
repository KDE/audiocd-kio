add_definitions(-DTRANSLATION_DOMAIN=\"kcmaudiocd\")

kcoreaddons_add_plugin(kcm_audiocd INSTALL_NAMESPACE "plasma/kcms/systemsettings_qwidgets")
kcmutils_generate_desktop_file(kcm_audiocd)

target_sources(kcm_audiocd PRIVATE
    kcmaudiocd.cpp kcmaudiocd.h
)

ki18n_wrap_ui(kcm_audiocd audiocdconfig.ui)

target_include_directories(kcm_audiocd
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(kcm_audiocd
    audiocdplugins 
    KF6::KCMUtils
    KF6::I18n
)

########### add handbook ################

if(KF6DocTools_FOUND)
    add_subdirectory(doc)
endif()
