INCLUDES = -I$(top_srcdir)/libkcddb -I$(srcdir)/.. $(all_includes)

kde_module_LTLIBRARIES = libaudiocd_encoder_flac.la

libaudiocd_encoder_flac_la_SOURCES = encoderflac.cpp

libaudiocd_encoder_flac_la_LIBADD = $(LIBFLAC) $(LIB_KIO) ../libaudiocdplugins.la

libaudiocd_encoder_flac_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)

pluginsdir = $(kde_datadir)/audiocd/plugins

METASOURCES = AUTO
