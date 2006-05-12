
include_directories( ${CMAKE_SOURCE_DIR}/libkcddb ${CMAKE_CURRENT_SOURCE_DIR}/..  )


########### next target ###############

set(audiocd_encoder_lame_PART_SRCS encoderlame.cpp collectingprocess.cpp )

kde4_automoc(${audiocd_encoder_lame_PART_SRCS})

kde4_add_ui3_files(audiocd_encoder_lame_PART_SRCS encoderlameconfig.ui )

kde4_add_kcfg_files(audiocd_encoder_lame_PART_SRCS audiocd_lame_encoder.kcfgc )

kde4_add_plugin(audiocd_encoder_lame WITH_PREFIX ${audiocd_encoder_lame_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} audiocd_encoder_lame )

target_link_libraries(audiocd_encoder_lame  ${KDE4_KIO_LIBS} audiocdplugins )

install_targets(${PLUGIN_INSTALL_DIR} audiocd_encoder_lame )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/audiocd/plugins FILES  )
install_files( ${KCFG_INSTALL_DIR} FILES audiocd_lame_encoder.kcfg )




#original Makefile.am contents follow:

#INCLUDES = -I$(top_srcdir)/libkcddb -I$(srcdir)/.. $(taglib_includes) $(all_includes)
#
#kde_kcfg_DATA = audiocd_lame_encoder.kcfg
#
#kde_module_LTLIBRARIES = libaudiocd_encoder_lame.la
#
#libaudiocd_encoder_lame_la_SOURCES = audiocd_lame_encoder.kcfgc encoderlame.cpp encoderlameconfig.ui collectingprocess.cpp
#
#libaudiocd_encoder_lame_la_LIBADD = $(LIB_KIO) ../libaudiocdplugins.la
#
#libaudiocd_encoder_lame_la_LDFLAGS = -avoid-version -module -no-undefined $(taglib_libs) $(all_libraries)
#
#pluginsdir = $(kde_datadir)/audiocd/plugins
#
#METASOURCES = AUTO
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/audiocd_encoder_lame.pot
