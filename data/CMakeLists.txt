install(FILES
  audiocd.desktop
  DESTINATION ${KDE_INSTALL_DATADIR}/konqsidebartng/virtual_folders/services)

install(FILES
  solid_audiocd.desktop
  DESTINATION ${KDE_INSTALL_DATADIR}/solid/actions)

install(FILES
  org.kde.kio_audiocd.metainfo.xml
  DESTINATION ${KDE_INSTALL_METAINFODIR})
