AM_CPPFLAGS = -I../ $(all_includes)

INCLUDES = $(LAME_INCS) $(all_includes)

kde_module_LTLIBRARIES = libaudiocd_encoder_wav.la

libaudiocd_encoder_wav_la_SOURCES = encoderwav.cpp encodercda.cpp

libaudiocd_encoder_wav_la_LIBADD = $(LIB_KIO) $(LIB_KDECORE) ../libaudiocdplugins.la

libaudiocd_encoder_wav_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/audiocd/plugins

METASOURCES = AUTO
