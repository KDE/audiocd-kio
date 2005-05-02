INCLUDES = -I$(srcdir)/.. $(LAME_INCS) $(taglib_includes) $(all_includes)

kde_module_LTLIBRARIES = libaudiocd_encoder_lame.la

libaudiocd_encoder_lame_la_SOURCES = encoderlame.cpp encoderlameconfig.ui

libaudiocd_encoder_lame_la_LIBADD = $(LIB_KIO) ../libaudiocdplugins.la \
				    ../../kcmaudiocd/audiocdsettings.lo

libaudiocd_encoder_lame_la_LDFLAGS = -avoid-version -module -no-undefined $(taglib_libs) $(all_libraries)

pluginsdir = $(kde_datadir)/audiocd/plugins

METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/audiocd_encoder_lame.pot
