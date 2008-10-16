project(audiocd)

add_definitions (-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)


MACRO_PUSH_REQUIRED_VARS()
SET(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} /usr/include/cdda)
check_struct_member("struct cdrom_drive" "ioctl_device_name" "cdda_interface.h" CDDA_IOCTL_DEVICE_EXISTS)
MACRO_POP_REQUIRED_VARS()
macro_bool_to_01(CDDA_IOCTL_DEVICE_EXISTS HAVE_CDDA_IOCTL_DEVICE)


configure_file (config-audiocd.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-audiocd.h )

#############################


add_subdirectory( plugins )
add_subdirectory( kcmaudiocd )
add_subdirectory( data )

include_directories(
		${CMAKE_SOURCE_DIR}/libkcddb
		${CMAKE_BINARY_DIR}/libkcddb
		${CMAKE_SOURCE_DIR}/libkcompactdisc
		${CMAKE_BINARY_DIR}/libkcompactdisc
		${CDPARANOIA_INCLUDE_DIR}
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

