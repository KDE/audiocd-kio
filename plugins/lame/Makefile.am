INCLUDES = -I$(srcdir)/.. $(LAME_INCS) $(all_includes)

kde_kcfg_DATA = audiocd_lame_encoder.kcfg

kde_module_LTLIBRARIES = libaudiocd_encoder_lame.la

libaudiocd_encoder_lame_la_SOURCES = audiocd_lame_encoder.kcfgc encoderlame.cpp encoderlameconfig.ui

libaudiocd_encoder_lame_la_LIBADD = $(LIB_KIO) $(LIB_KDECORE) ../libaudiocdplugins.la

libaudiocd_encoder_lame_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/audiocd/plugins

METASOURCES = AUTO
