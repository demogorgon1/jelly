cmake_minimum_required (VERSION 3.19)

project(jelly)

macro(jelly_option OPTION_NAME OPTION_TEXT OPTION_DEFAULT)
    option(${OPTION_NAME} ${OPTION_TEXT} ${OPTION_DEFAULT})
    if(DEFINED ENV{${OPTION_NAME}})
        # Allow overriding the option through an environment variable
        set(${OPTION_NAME} $ENV{${OPTION_NAME}})
    endif()
    if(${OPTION_NAME})
        add_definitions(-D${OPTION_NAME})
    endif()
    message(STATUS "${OPTION_NAME}: ${${OPTION_NAME}}")
endmacro()

jelly_option(JELLY_ZSTD "Use ZSTD." OFF)
jelly_option(JELLY_PRECOMPILED_HEADERS "Enable precompiled headers." ON)

enable_testing()

include(FetchContent)
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  ${CMAKE_MODULE_PATH})

# --------------------------------------------------------------------

if (JELLY_ZSTD)
	find_package(ZSTD REQUIRED)
endif()

# --------------------------------------------------------------------

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unused-function -Wno-reorder)
endif()

add_subdirectory(jelly)
add_subdirectory(test)
add_subdirectory(include)
