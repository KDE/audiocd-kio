add_definitions(-DTRANSLATION_DOMAIN=\"kcmaudiocd\")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../plugins
    ${CMAKE_BINARY_DIR}/kioslave/audiocd/kcmaudiocd
)

########### next target ###############

set(kcm_audiocd_PART_SRCS kcmaudiocd.cpp)

ki18n_wrap_ui(kcm_audiocd_PART_SRCS audiocdconfig.ui)

add_library(kcm_audiocd MODULE ${kcm_audiocd_PART_SRCS})

target_link_libraries(kcm_audiocd
    KF5::I18n
    KF5::KCMUtils
    audiocdplugins 
)

install(TARGETS kcm_audiocd  DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install(FILES audiocd.desktop  DESTINATION  ${SERVICES_INSTALL_DIR})

########### add handbook ################

add_subdirectory(doc)
