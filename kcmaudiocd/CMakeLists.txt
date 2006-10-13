
include_directories( ${CMAKE_SOURCE_DIR}/libkcddb ${CMAKE_CURRENT_SOURCE_DIR}/../plugins/   )


########### next target ###############

set(kcm_audiocd_PART_SRCS kcmaudiocd.cpp )

kde4_automoc(kcm_audiocd ${kcm_audiocd_PART_SRCS})

kde4_add_ui3_files(kcm_audiocd_PART_SRCS audiocdconfig.ui )

kde4_add_plugin(kcm_audiocd ${kcm_audiocd_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_audiocd )

target_link_libraries(kcm_audiocd  audiocdplugins ${KDE4_KDEUI_LIBS} )

install(TARGETS kcm_audiocd  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES audiocd.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

