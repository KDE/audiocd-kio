kde_module_LTLIBRARIES = kcm_audiocd.la

kcm_audiocd_la_SOURCES = audiocdconfig.ui kcmaudiocd.cpp

kcm_audiocd_la_LDFLAGS  =  $(all_libraries) $(KDE_PLUGIN) -module
kcm_audiocd_la_LIBADD = $(LIB_KDEUI)

INCLUDES = $(all_includes)

kcm_audiocd_la_METASOURCES = AUTO

KDE_ICON = cdrom_unmount

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kcmaudiocd.pot

kioslave_audiocd_DATA = audiocd.desktop
kioslave_audiocddir = $(kde_appsdir)/Settings/Sound

audiocdconfig.cpp: audiocdconfig.h
