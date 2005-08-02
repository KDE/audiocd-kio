if include_kcm_audiocd
AUDIO_CD_SUBDIRS = kcmaudiocd
endif
SUBDIRS = plugins $(AUDIO_CD_SUBDIRS)

INCLUDES = -I$(top_srcdir)/libkcddb \
		   -I$(top_builddir)/libkcddb $(all_includes) \
		   -I$(top_srcdir)/kscd $(all_includes) \
		   -I$(srcdir)/plugins $(all_includes)

KDE_CXXFLAGS=$(ENABLE_PERMISSIVE_FLAG)

kde_module_LTLIBRARIES = kio_audiocd.la

kio_audiocd_la_SOURCES = audiocd.cpp

kio_audiocd_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

kio_audiocd_la_LIBADD = ./plugins/libaudiocdplugins.la $(LIB_KIO) $(CDPARANOIA_LIBS) $(top_builddir)/libkcddb/libkcddb.la $(top_builddir)/kscd/libkcompactdisc.la 

noinst_HEADERS = audiocd.h

protocoldir = $(kde_servicesdir)
protocol_DATA = audiocd.protocol

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kio_audiocd.pot

updatedir = $(kde_datadir)/kconf_update
update_DATA = audiocd.upd
update_SCRIPTS = upgrade-metadata.sh

audiocd.lo: ../../libkcddb/configbase.h
