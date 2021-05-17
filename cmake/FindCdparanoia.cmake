# - Try to find the CD Paranoia libraries
# Once done this will define
#
#  CDPARANOIA_FOUND       - system has cdparanoia
#  CDPARANOIA_INCLUDE_DIR - the cdparanoia include directory
#  CDPARANOIA_LIBRARIES   - Link these to use cdparanoia
#

# Copyright (c) 2006, Richard Laerkaeng, <richard@goteborg.utfors.se>
# Copyright (c) 2007, Allen Winter, <winter@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

find_path(CDPARANOIA_INCLUDE_DIR cdda_interface.h PATH_SUFFIXES cdda cdparanoia)

find_library(CDPARANOIA_LIBRARY NAMES cdda_paranoia)
find_library(CDPARANOIA_IF_LIBRARY NAMES cdda_interface)

if (CDPARANOIA_LIBRARY AND CDPARANOIA_IF_LIBRARY)
    SET(CDPARANOIA_LIBRARIES ${CDPARANOIA_LIBRARY} ${CDPARANOIA_IF_LIBRARY} "-lm")
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Cdparanoia
    DEFAULT_MSG
    CDPARANOIA_LIBRARIES CDPARANOIA_INCLUDE_DIR
)

mark_as_advanced(
    CDPARANOIA_INCLUDE_DIR
    CDPARANOIA_LIBRARY
    CDPARANOIA_IF_LIBRARY
)

if(CDPARANOIA_FOUND AND NOT TARGET Cdparanoia::Cdparanoia)
    add_library(Cdparanoia::Cdparanoia UNKNOWN IMPORTED)
    set_target_properties(Cdparanoia::Cdparanoia PROPERTIES
        IMPORTED_LOCATION "${CDPARANOIA_LIBRARY}"
        INTERFACE_LINK_LIBRARIES  "${CDPARANOIA_IF_LIBRARY};-lm"
        INTERFACE_INCLUDE_DIRECTORIES "${CDPARANOIA_INCLUDE_DIR}"
    )
endif()
