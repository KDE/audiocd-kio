if include_kcm_audiocd
AUDIO_CD_SUBDIRS = kcmaudiocd
endif
SUBDIRS = plugins $(AUDIO_CD_SUBDIRS)

INCLUDES = $(CDPARANOIA_INCS) -I$(top_srcdir)/libkcddb -Iplugins $(all_includes)

kde_module_LTLIBRARIES = kio_audiocd.la

kio_audiocd_la_SOURCES = audiocd.cpp

kio_audiocd_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(CDPARANOIA_RPATH) \
 $(LAME_RPATH) $(VORBIS_RPATH) -module $(KDE_PLUGIN)

kio_audiocd_la_LIBADD = ./plugins/libaudiocdplugins.la $(LIB_KIO) $(CDPARANOIA_LIBS) $(top_builddir)/libkcddb/libkcddb.la

noinst_HEADERS = audiocd.h

protocoldir = $(kde_servicesdir)
protocol_DATA = audiocd.protocol

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kio_audiocd.pot
