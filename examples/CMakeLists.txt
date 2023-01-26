cmake_minimum_required (VERSION 3.19)

add_executable(example-blob-node ExampleBlobNode.cpp)
	
target_compile_features(example-blob-node PRIVATE cxx_std_20)

if(JELLY_PRECOMPILED_HEADERS)
	target_precompile_headers(example-blob-node PRIVATE <jelly/API.h>)
endif()

target_link_libraries(example-blob-node jelly::jelly)

if(NOT MSVC)
	target_link_libraries(example-blob-node pthread)
endif()
