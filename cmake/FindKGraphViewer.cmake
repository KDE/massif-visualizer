# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.  A copy of the full
# GNU Lesser General Public License can be found at
# http://www.gnu.org/copyleft/lesser.html.
#
# For more information, contact the authors of this software at
# Milian Wolff <mail@milianw.de>
#
# this will set the following variables:
# KGRAPHVIEWER_LIBRARIES
# KGRAPHVIEWER_FOUND
# KGRAPHVIEWER_INCLUDE_DIRECTORIES

# if ( NOT WIN32 )

#   find_package(PkgConfig)
#   pkg_check_modules( temp ${REQUIRED} kgraphviewer )
#   if ( temp_FOUND )
#     set ( KGRAPHVIEWER_INCLUDE_DIRECTORIES ${temp_INCLUDE_DIRS} )
#   endif ( temp_FOUND )

# endif ( NOT WIN32 )

find_path( KGRAPHVIEWER_INCLUDE_DIRECTORIES
    NAMES kgraphviewer_interface.h
    PATHS
    ${KGRAPHVIEWER_INCLUDE_DIRS}
    /usr/local/include
    /usr/include
    PATH_SUFFIXES kgraphviewer
    )

find_library( KGRAPHVIEWER_LIBRARIES
    NAMES kgraphviewer
    PATHS
    ${KGRAPHVIEWER_LIBRARY_DIRS}
    /usr/local/lib64
    /usr/lib64
    /usr/local/lib
    /usr/lib
    )

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set KGRAPHVIEWER_FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(KGRAPHVIEWER DEFAULT_MSG KGRAPHVIEWER_LIBRARIES KGRAPHVIEWER_INCLUDE_DIRECTORIES)