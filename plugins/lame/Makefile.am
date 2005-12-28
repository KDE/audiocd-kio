INCLUDES = -I$(top_srcdir)/libkcddb -I$(srcdir)/.. $(taglib_includes) $(all_includes)

kde_kcfg_DATA = audiocd_lame_encoder.kcfg

kde_module_LTLIBRARIES = libaudiocd_encoder_lame.la

libaudiocd_encoder_lame_la_SOURCES = audiocd_lame_encoder.kcfgc encoderlame.cpp encoderlameconfig.ui collectingprocess.cpp

libaudiocd_encoder_lame_la_LIBADD = $(LIB_KIO) ../libaudiocdplugins.la

libaudiocd_encoder_lame_la_LDFLAGS = -avoid-version -module -no-undefined $(taglib_libs) $(all_libraries)

pluginsdir = $(kde_datadir)/audiocd/plugins

METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/audiocd_encoder_lame.pot
