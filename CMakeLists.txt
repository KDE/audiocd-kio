project(audiocd)

add_definitions (-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)

add_subdirectory( plugins )
add_subdirectory( kcmaudiocd )

include_directories(
		${CMAKE_SOURCE_DIR}/libkcddb
		${CMAKE_BINARY_DIR}/libkcddb
		${CMAKE_SOURCE_DIR}/libkcompactdisc
		${CMAKE_BINARY_DIR}/libkcompactdisc
		)


########### next target ###############

set(kio_audiocd_PART_SRCS audiocd.cpp )


kde4_add_plugin(kio_audiocd ${kio_audiocd_PART_SRCS})



target_link_libraries(kio_audiocd ${CDPARANOIA_LIBRARIES} ${KDE4_KIO_LIBS} audiocdplugins kcddb kcompactdisc)

install(TARGETS kio_audiocd  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES audiocd.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
if(NOT WIN32)
  install( FILES audiocd.upd  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
  install( PROGRAMS upgrade-metadata.sh  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
endif(NOT WIN32)

