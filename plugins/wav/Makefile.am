AM_CPPFLAGS = -I$(srcdir)/.. $(CDPARANOIA_INCS) $(all_includes)

kde_module_LTLIBRARIES = libaudiocd_encoder_wav.la

libaudiocd_encoder_wav_la_SOURCES = encoderwav.cpp encodercda.cpp

libaudiocd_encoder_wav_la_LIBADD = $(LIB_KIO) ../libaudiocdplugins.la $(CDPARANOIA_LIBS)

libaudiocd_encoder_wav_la_LDFLAGS = $(CDPARANOIA_RPATH) -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/audiocd/plugins

METASOURCES = AUTO
