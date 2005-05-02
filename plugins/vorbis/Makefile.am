INCLUDES = -I$(srcdir)/.. $(VORBIS_INCS) $(all_includes)

kde_module_LTLIBRARIES = libaudiocd_encoder_vorbis.la

libaudiocd_encoder_vorbis_la_SOURCES = encodervorbis.cpp encodervorbisconfig.ui

libaudiocd_encoder_vorbis_la_LIBADD = $(VORBIS_LIBS) $(VORBISFILE_LIBS) $(VORBISENC_LIBS) $(LIB_KIO) \
				      ../libaudiocdplugins.la ../../kcmaudiocd/audiocdsettings.lo

libaudiocd_encoder_vorbis_la_LDFLAGS = $(VORBIS_RPATH) -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/audiocd/plugins

METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/audiocd_encoder_vorbis.pot
