
include_directories( ${CMAKE_SOURCE_DIR}/libkcddb ${CMAKE_CURRENT_SOURCE_DIR}/..   )


########### next target ###############

set(audiocd_encoder_wav_PART_SRCS encoderwav.cpp encodercda.cpp )

kde4_automoc(${audiocd_encoder_wav_PART_SRCS})

kde4_add_plugin(audiocd_encoder_wav WITH_PREFIX ${audiocd_encoder_wav_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} audiocd_encoder_wav )

target_link_libraries(audiocd_encoder_wav  ${KDE4_KIO_LIBS} audiocdplugins )

install(TARGETS audiocd_encoder_wav  DESTINATION ${PLUGIN_INSTALL_DIR} )





#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(srcdir)/.. $(all_includes)
#
#INCLUDES = -I$(top_srcdir)/libkcddb
#
#kde_module_LTLIBRARIES = libaudiocd_encoder_wav.la
#
#libaudiocd_encoder_wav_la_SOURCES = encoderwav.cpp encodercda.cpp
#
#libaudiocd_encoder_wav_la_LIBADD = $(LIB_KIO) ../libaudiocdplugins.la $(CDPARANOIA_LIBS)
#
#libaudiocd_encoder_wav_la_LDFLAGS = -avoid-version -module -no-undefined $(all_libraries)
#
#pluginsdir = $(kde_datadir)/audiocd/plugins
#
#METASOURCES = AUTO
