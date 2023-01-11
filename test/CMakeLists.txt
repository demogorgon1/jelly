cmake_minimum_required (VERSION 3.19)

file(GLOB_RECURSE C_FILES "*.cpp")
file(GLOB_RECURSE H_FILES "*.h")

add_executable(jelly-test ${C_FILES} ${H_FILES})
	
target_compile_features(jelly-test PRIVATE cxx_std_20)

if(JELLY_PRECOMPILED_HEADERS)
	target_precompile_headers(jelly-test PRIVATE <jelly/API.h>)
endif()

target_link_libraries(jelly-test jelly::jelly)

if(NOT MSVC)
	target_link_libraries(jelly-test pthread)
endif()

add_test(jelly-test jelly-test)