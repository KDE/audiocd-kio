
include_directories( ${CMAKE_SOURCE_DIR}/libkcddb ${CMAKE_CURRENT_SOURCE_DIR}/..  ${FLAC_INCLUDE_DIR} )


########### next target ###############

# Remove this, and #ifdef in encoderflac when Makefile.am's are removed
add_definitions(-DHAVE_LIBFLAC)

set(audiocd_encoder_flac_PART_SRCS encoderflac.cpp )

kde4_automoc(${audiocd_encoder_flac_PART_SRCS})

kde4_add_plugin(audiocd_encoder_flac WITH_PREFIX ${audiocd_encoder_flac_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} audiocd_encoder_flac )

target_link_libraries(audiocd_encoder_flac ${FLAC_LIBRARIES} ${KDE4_KIO_LIBS} audiocdplugins )

install(TARGETS audiocd_encoder_flac  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES   DESTINATION  ${DATA_INSTALL_DIR}/audiocd/plugins )




#original Makefile.am contents follow:

#INCLUDES = -I$(top_srcdir)/libkcddb -I$(srcdir)/.. $(all_includes)
#
#kde_module_LTLIBRARIES = libaudiocd_encoder_flac.la
#
#libaudiocd_encoder_flac_la_SOURCES = encoderflac.cpp
#
#libaudiocd_encoder_flac_la_LIBADD = $(LIBFLAC) $(LIB_KIO) ../libaudiocdplugins.la
#
#libaudiocd_encoder_flac_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)
#
#pluginsdir = $(kde_datadir)/audiocd/plugins
#
#METASOURCES = AUTO
