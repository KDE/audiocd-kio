kde4_header()

include_directories( ${CMAKE_SOURCE_DIR}/libkcddb ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(audiocd_encoder_flac_PART_SRCS encoderflac.cpp )

kde4_automoc(${audiocd_encoder_flac_PART_SRCS})

kde4_add_plugin(audiocd_encoder_flac WITH_PREFIX ${audiocd_encoder_flac_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} audiocd_encoder_flac )

target_link_libraries(audiocd_encoder_flac  ${QT_AND_KDECORE_LIBS} kio audiocdplugins )

install_targets(${LIB_INSTALL_DIR}/kde4 audiocd_encoder_flac )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/audiocd/plugins FILES  )

kde4_footer()



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
