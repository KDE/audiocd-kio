project(audiocd)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(CheckStructMember)

find_package(Libkcddb)
find_package(Libkcompactdisc)

find_package(Cdparanoia)

MACRO_PUSH_REQUIRED_VARS()
SET(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${CDPARANOIA_INCLUDE_DIR})
check_struct_member("struct cdrom_drive" "ioctl_device_name" "cdda_interface.h" CDDA_IOCTL_DEVICE_EXISTS)
MACRO_POP_REQUIRED_VARS()
macro_bool_to_01(CDDA_IOCTL_DEVICE_EXISTS HAVE_CDDA_IOCTL_DEVICE)


configure_file (config-audiocd.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-audiocd.h )

#############################

include_directories(
    ${KDE4_INCLUDES}
		${CDPARANOIA_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LIBKCDDB_INCLUDE_DIRS}
    ${LIBKCOMPACTDISC_INCLUDE_DIRS}
		)


add_subdirectory( plugins )
add_subdirectory( kcmaudiocd )
add_subdirectory( data )
add_subdirectory( doc )


########### next target ###############

set(kio_audiocd_PART_SRCS audiocd.cpp )


kde4_add_plugin(kio_audiocd ${kio_audiocd_PART_SRCS})



target_link_libraries(kio_audiocd ${CDPARANOIA_LIBRARIES} ${KDE4_KIO_LIBS} ${LIBKCDDB_LIBRARIES} ${LIBKCOMPACTDISC_LIBRARIES} audiocdplugins)

install(TARGETS kio_audiocd  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES audiocd.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

