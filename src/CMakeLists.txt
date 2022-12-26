cmake_minimum_required(VERSION 3.15)

include_directories(
	include
)

file(GLOB C_FILES "*.cpp")
file(GLOB H_FILES "*.h")

add_library(jelly STATIC ${C_FILES} ${H_FILES})

add_library(jelly::jelly ALIAS jelly)

target_compile_features(jelly PRIVATE cxx_std_17)

install(TARGETS jelly
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
