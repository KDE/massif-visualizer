#
# Find the KGraphViewer library and sets various variables accordingly
#
# Example usage of this module:
# find_package(KGraphViewer 2.1 REQUIRED)
#
# The version number and REQUIRED flag are optional. You can set CMAKE_PREFIX_PATH
# variable to help it find the required files and directories
#
# this will set the following variables:
# KGRAPHVIEWER_LIBRARIES           - KGraphViewer library
# KGRAPHVIEWER_FOUND               - Whether KGraphViewer was found
# KGRAPHVIEWER_INCLUDE_DIRECTORIES - Include directories for the KGraphViewer library
#
# Copyright 2010 Milian Wolff <mail@milianw.de>
# Redistribution and use is allowed according to the terms of the BSD license.

find_path( KGRAPHVIEWER_INCLUDE_DIRECTORIES
    NAMES kgraphviewer_interface.h
    HINTS
    ${KGRAPHVIEWER_INCLUDE_DIRS}
    /usr/local/include
    /usr/include
    PATH_SUFFIXES kgraphviewer
    )

find_library( KGRAPHVIEWER_LIBRARIES
    NAMES kgraphviewer
    HINTS
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