kde4_header()

include_directories( ${CMAKE_SOURCE_DIR}/libkcddb ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(audiocd_encoder_vorbis_PART_SRCS encodervorbis.cpp )

kde4_automoc(${audiocd_encoder_vorbis_PART_SRCS})

kde4_add_ui_files(audiocd_encoder_vorbis_PART_SRCS encodervorbisconfig.ui )

kde4_add_kcfg_files(audiocd_encoder_vorbis_PART_SRCS audiocd_vorbis_encoder.kcfgc )

kde4_add_plugin(audiocd_encoder_vorbis WITH_PREFIX ${audiocd_encoder_vorbis_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} audiocd_encoder_vorbis )

target_link_libraries(audiocd_encoder_vorbis  ${QT_AND_KDECORE_LIBS} kio audiocdplugins )

install_targets(${LIB_INSTALL_DIR}/kde4 audiocd_encoder_vorbis )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/audiocd/plugins FILES  )
install_files( ${KCFG_INSTALL_DIR} FILES audiocd_vorbis_encoder.kcfg )

kde4_footer()



#original Makefile.am contents follow:

#INCLUDES = -I$(top_srcdir)/libkcddb -I$(srcdir)/.. $(all_includes)
#
#kde_kcfg_DATA = audiocd_vorbis_encoder.kcfg
#
#kde_module_LTLIBRARIES = libaudiocd_encoder_vorbis.la
#
#libaudiocd_encoder_vorbis_la_SOURCES = audiocd_vorbis_encoder.kcfgc encodervorbis.cpp encodervorbisconfig.ui
#
#libaudiocd_encoder_vorbis_la_LIBADD = $(VORBIS_LIBS) $(VORBISFILE_LIBS) $(VORBISENC_LIBS) $(LIB_KIO) ../libaudiocdplugins.la
#
#libaudiocd_encoder_vorbis_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)
#
#pluginsdir = $(kde_datadir)/audiocd/plugins
#
#METASOURCES = AUTO
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/audiocd_encoder_vorbis.pot
