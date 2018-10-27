install(FILES
  audiocd.desktop
  DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/virtual_folders/services)

install(FILES
  solid_audiocd.desktop
  DESTINATION ${DATA_INSTALL_DIR}/solid/actions)

install(FILES
  org.kde.kio_audiocd.appdata.xml
  DESTINATION ${KDE_INSTALL_METAINFODIR})
