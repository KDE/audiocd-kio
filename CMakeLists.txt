project(audiocd)

add_subdirectory( plugins ) 
add_subdirectory( kcmaudiocd )

include_directories( 
		${CMAKE_SOURCE_DIR}/libkcddb 
		${CMAKE_BINARY_DIR}/libkcddb 
		${CMAKE_SOURCE_DIR}/kscd   
		${CMAKE_BINARY_DIR}/kscd
		)


########### next target ###############

set(kio_audiocd_PART_SRCS ${libkcompactdisc_SRCS} ${libworkman_SRCS} audiocd.cpp )

kde4_automoc(${kio_audiocd_PART_SRCS})

kde4_add_plugin(kio_audiocd ${kio_audiocd_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_audiocd )

target_link_libraries(kio_audiocd ${CDPARANOIA_LIBRARIES} ${KDE4_KIO_LIBS} audiocdplugins kcddb )

install(TARGETS kio_audiocd  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES audiocd.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES audiocd.upd  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
install( PROGRAMS upgrade-metadata.sh  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )

