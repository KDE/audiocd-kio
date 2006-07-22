
include_directories( ${CMAKE_SOURCE_DIR}/libkcddb ${CMAKE_CURRENT_SOURCE_DIR}/../plugins/   )


########### next target ###############

set(kcm_audiocd_PART_SRCS kcmaudiocd.cpp )

kde4_automoc(${kcm_audiocd_PART_SRCS})

kde4_add_ui3_files(kcm_audiocd_PART_SRCS audiocdconfig.ui )

kde4_add_plugin(kcm_audiocd ${kcm_audiocd_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_audiocd )

target_link_libraries(kcm_audiocd  ${KDE4_KDECORE_LIBS} audiocdplugins ${KDE4_KDEUI_LIBS} )

install(TARGETS kcm_audiocd  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES audiocd.desktop  DESTINATION  ${XDG_APPS_DIR} )




#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = kcm_audiocd.la
#
#kcm_audiocd_la_SOURCES = audiocdconfig.ui kcmaudiocd.cpp
#
#kcm_audiocd_la_LDFLAGS  =  $(all_libraries) $(KDE_PLUGIN) -module
#kcm_audiocd_la_LIBADD = ../plugins/libaudiocdplugins.la $(LIB_KDEUI)
#
#INCLUDES = -I$(srcdir)/../plugins -I$(top_srcdir)/libkcddb $(all_includes)
#
#kcm_audiocd_la_METASOURCES = AUTO
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/kcmaudiocd.pot
#
#xdg_apps_DATA = audiocd.desktop
#
#audiocdconfig.cpp: audiocdconfig.h
