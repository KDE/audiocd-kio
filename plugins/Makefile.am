#if HAVE_VORBIS
#AUDIOCD_PLUGINS_SUBDIR=vorbis
#endif

SUBDIRS = . wav vorbis lame flac

INCLUDES = $(all_includes)

lib_LTLIBRARIES = libaudiocdplugins.la

libaudiocdplugins_la_LIBADD =  $(LIB_QT) $(LIB_KDECORE)

libaudiocdplugins_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -no-undefined

libaudiocdplugins_la_SOURCES = 	audiocdencoder.cpp

include_HEADERS = audiocdencoder.h

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kio_audiocd.pot

