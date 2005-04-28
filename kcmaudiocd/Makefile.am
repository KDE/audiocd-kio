kde_module_LTLIBRARIES = kcm_audiocd.la

kcm_audiocd_la_SOURCES = audiocdconfig.ui kcmaudiocd.cpp audiocdsettings.kcfgc

kcm_audiocd_la_LDFLAGS  =  $(all_libraries) $(KDE_PLUGIN) -module
kcm_audiocd_la_LIBADD = ../plugins/libaudiocdplugins.la $(LIB_KDEUI)

INCLUDES = -I$(srcdir)/../plugins $(all_includes)

kcm_audiocd_la_METASOURCES = AUTO

kde_kcfg_DATA = audiocdsettings.kcfg

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kcmaudiocd.pot

xdg_apps_DATA = audiocd.desktop

audiocdconfig.cpp: audiocdconfig.h
