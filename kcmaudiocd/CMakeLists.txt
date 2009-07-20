
include_directories(
                ${CMAKE_SOURCE_DIR}/libkcddb
                ${CMAKE_CURRENT_SOURCE_DIR}/../plugins
                ${CMAKE_BINARY_DIR}/kioslave/audiocd/kcmaudiocd )

########### next target ###############

set(kcm_audiocd_PART_SRCS kcmaudiocd.cpp )


kde4_add_ui_files(kcm_audiocd_PART_SRCS audiocdconfig.ui )

kde4_add_plugin(kcm_audiocd ${kcm_audiocd_PART_SRCS})



target_link_libraries(kcm_audiocd  audiocdplugins ${KDE4_KDEUI_LIBS} )

install(TARGETS kcm_audiocd  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES audiocd.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

