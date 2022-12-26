cmake_minimum_required (VERSION 3.19)

file(GLOB C_FILES "*.cpp")
file(GLOB H_FILES "*.h")

add_executable(jelly-test ${C_FILES} ${H_FILES})
	
target_compile_features(jelly-test PRIVATE cxx_std_17)
target_link_libraries(jelly-test jelly::jelly)

add_test(jelly-test jelly-test)