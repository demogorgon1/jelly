find_path(JELLY_INCLUDE_DIR
    NAMES sfc.h)

find_library(JELLY_LIBRARY
	NAMES jelly
    HINTS ${SFC_LIBRARY_ROOT})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JELLY REQUIRED_VARS JELLY_LIBRARY JELLY_INCLUDE_DIR)

if(JELLY_FOUND)
    set(JELLY_LIBRARIES ${SFC_LIBRARY})
    set(JELLY_INCLUDE_DIRS ${SFC_INCLUDE_DIR})
endif()