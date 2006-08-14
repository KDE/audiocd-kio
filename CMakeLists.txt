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



#original Makefile.am contents follow:

#if include_kcm_audiocd
#AUDIO_CD_SUBDIRS = kcmaudiocd
#endif
#SUBDIRS = plugins $(AUDIO_CD_SUBDIRS)
#
#INCLUDES = -I$(top_srcdir)/libkcddb \
#		   -I$(top_builddir)/libkcddb $(all_includes) \
#		   -I$(top_srcdir)/kscd $(all_includes) \
#		   -I$(srcdir)/plugins $(all_includes)
#
#KDE_CXXFLAGS=$(ENABLE_PERMISSIVE_FLAG)
#
#kde_module_LTLIBRARIES = kio_audiocd.la
#
#kio_audiocd_la_SOURCES = audiocd.cpp
#
#kio_audiocd_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#
#kio_audiocd_la_LIBADD = ./plugins/libaudiocdplugins.la $(LIB_KIO) $(CDPARANOIA_LIBS) $(top_builddir)/libkcddb/libkcddb.la $(top_builddir)/kscd/libkcompactdisc.la 
#
#noinst_HEADERS = audiocd.h
#
#protocoldir = $(kde_servicesdir)
#protocol_DATA = audiocd.protocol
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/kio_audiocd.pot
#
#updatedir = $(kde_datadir)/kconf_update
#update_DATA = audiocd.upd
#update_SCRIPTS = upgrade-metadata.sh
#
#audiocd.lo: ../../libkcddb/configbase.h
