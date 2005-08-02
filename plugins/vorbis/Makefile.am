INCLUDES = -I$(top_srcdir)/libkcddb -I$(srcdir)/.. $(all_includes)

kde_kcfg_DATA = audiocd_vorbis_encoder.kcfg

kde_module_LTLIBRARIES = libaudiocd_encoder_vorbis.la

libaudiocd_encoder_vorbis_la_SOURCES = audiocd_vorbis_encoder.kcfgc encodervorbis.cpp encodervorbisconfig.ui

libaudiocd_encoder_vorbis_la_LIBADD = $(VORBIS_LIBS) $(VORBISFILE_LIBS) $(VORBISENC_LIBS) $(LIB_KIO) ../libaudiocdplugins.la

libaudiocd_encoder_vorbis_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/audiocd/plugins

METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/audiocd_encoder_vorbis.pot
