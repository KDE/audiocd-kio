kde4_header()

add_subdirectory( wav ) 
add_subdirectory( vorbis ) 
add_subdirectory( lame ) 
add_subdirectory( flac ) 

include_directories( ${CMAKE_SOURCE_DIR}/libkcddb ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(audiocdplugins_LIB_SRCS audiocdencoder.cpp )

kde4_automoc(${audiocdplugins_LIB_SRCS})

kde4_add_library(audiocdplugins SHARED ${audiocdplugins_LIB_SRCS})

target_link_libraries(audiocdplugins  ${KDE4_KDECORE_LIBS} kcddb )

set_target_properties(audiocdplugins PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install_targets(${LIB_INSTALL_DIR} audiocdplugins )


########### install files ###############

install_files( /include FILES audiocdencoder.h )

kde4_footer()



#original Makefile.am contents follow:

##if HAVE_VORBIS
##AUDIOCD_PLUGINS_SUBDIR=vorbis
##endif
#
#SUBDIRS = . wav vorbis lame flac
#
#INCLUDES = -I$(top_srcdir)/libkcddb $(all_includes)
#		   
#lib_LTLIBRARIES = libaudiocdplugins.la
#
#libaudiocdplugins_la_LIBADD =  $(LIB_KDECORE) $(top_builddir)/libkcddb/libkcddb.la
#
#libaudiocdplugins_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -no-undefined
#
#libaudiocdplugins_la_SOURCES = 	audiocdencoder.cpp
#
#include_HEADERS = audiocdencoder.h
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/kio_audiocd.pot
#
