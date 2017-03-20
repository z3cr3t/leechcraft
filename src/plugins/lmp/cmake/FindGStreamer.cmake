# - Try to find GStreamer
# Once done this will define
#
#  GSTREAMER_FOUND - system has GStreamer
#  GSTREAMER_INCLUDE_DIR - the GStreamer include directory
#  GSTREAMER_LIBRARIES - the libraries needed to use GStreamer
#  GSTREAMER_DEFINITIONS - Compiler switches required for using GStreamer

# Copyright (c) 2006, Tim Beaulen <tbscope@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# TODO: Other versions --> GSTREAMER_X_Y_FOUND (Example: GSTREAMER_0_8_FOUND and GSTREAMER_0_10_FOUND etc)

IF (GSTREAMER_INCLUDE_DIR AND GSTREAMER_LIBRARIES AND GSTREAMER_BASE_LIBRARY)
   # in cache already
   SET(GStreamer_FIND_QUIETLY TRUE)
ELSE (GSTREAMER_INCLUDE_DIR AND GSTREAMER_LIBRARIES AND GSTREAMER_BASE_LIBRARY)
   SET(GStreamer_FIND_QUIETLY FALSE)
ENDIF (GSTREAMER_INCLUDE_DIR AND GSTREAMER_LIBRARIES AND GSTREAMER_BASE_LIBRARY)

IF (NOT WIN32)
	# use pkg-config to get the directories and then use these values
	# in the FIND_PATH() and FIND_LIBRARY() calls
	FIND_PACKAGE(PkgConfig)
	PKG_CHECK_MODULES(PC_GSTREAMER gstreamer-1.0)
	#MESSAGE(STATUS "DEBUG: GStreamer include directory = ${PC_GSTREAMER_INCLUDE_DIRS}")
	#MESSAGE(STATUS "DEBUG: GStreamer link directory = ${GSTREAMER_LIBRARY_DIRS}")
	#MESSAGE(STATUS "DEBUG: GStreamer CFlags = ${GSTREAMER_CFLAGS_OTHER}")
	SET(GSTREAMER_DEFINITIONS ${PC_GSTREAMER_CFLAGS_OTHER})
ENDIF (NOT WIN32)

MESSAGE (STATUS "Searching for GStreamer 1.0...")
FIND_LIBRARY(GSTREAMER_LIBRARIES NAMES gstreamer-1.0
	PATHS
		${PC_GSTREAMER_LIBDIR}
		${PC_GSTREAMER_LIBRARY_DIRS}
		${GSTREAMER_DIR}/lib
	)

FIND_LIBRARY(GSTREAMER_BASE_LIBRARY NAMES gstbase-1.0
	PATHS
		${PC_GSTREAMER_LIBDIR}
		${PC_GSTREAMER_LIBRARY_DIRS}
		${GSTREAMER_DIR}/lib
	)

FIND_PATH(GSTREAMER_INCLUDE_DIR gst/gst.h
	HINTS
		${PC_GSTREAMER_INCLUDEDIR}
		${PC_GSTREAMER_INCLUDE_DIRS}
		${GSTREAMER_DIR}/include/gstreamer-1.0
	PATHS
		/usr/local/include/gstreamer-1.0
		/usr/include/gstreamer-1.0
	#PATH_SUFFIXES gst
	)

IF (GSTREAMER_INCLUDE_DIR AND PC_GSTREAMER_INCLUDE_DIRS)
	SET (GSTREAMER_INCLUDE_DIR ${GSTREAMER_INCLUDE_DIR} ${PC_GSTREAMER_INCLUDE_DIRS})
ENDIF ()

IF (GSTREAMER_INCLUDE_DIR)
   MESSAGE(STATUS "DEBUG: Found GStreamer include dir: ${GSTREAMER_INCLUDE_DIR}")
ELSE ()
   MESSAGE(STATUS "GStreamer: WARNING: include dir not found")
ENDIF ()

IF (GSTREAMER_LIBRARIES)
   #MESSAGE(STATUS "DEBUG: Found GStreamer library: ${GSTREAMER_LIBRARIES}")
ELSE ()
   MESSAGE(STATUS "GStreamer: WARNING: library not found")
ENDIF ()

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(GStreamer  DEFAULT_MSG  GSTREAMER_LIBRARIES GSTREAMER_INCLUDE_DIR GSTREAMER_BASE_LIBRARY)

MARK_AS_ADVANCED(GSTREAMER_INCLUDE_DIR GSTREAMER_LIBRARIES GSTREAMER_BASE_LIBRARY)
