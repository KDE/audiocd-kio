if include_kcm_audiocd
SUBDIRS = kcmaudiocd
endif

INCLUDES = $(CDPARANOIA_INCS) $(LAME_INCS) $(VORBIS_INCS) -I$(top_srcdir)/libkcddb $(all_includes)

kde_module_LTLIBRARIES = kio_audiocd.la

kio_audiocd_la_SOURCES = audiocd.cpp

kio_audiocd_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(CDPARANOIA_RPATH) \
 $(LAME_RPATH) $(VORBIS_RPATH) -module $(KDE_PLUGIN)

kio_audiocd_la_LIBADD = $(LIB_KIO) $(CDPARANOIA_LIBS) $(VORBISENC_LIBS) $(VORBIS_LIBS) $(top_builddir)/libkcddb/libkcddb.la

noinst_HEADERS = audiocd.h

protocoldir = $(kde_servicesdir)
protocol_DATA = audiocd.protocol

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kio_audiocd.pot
