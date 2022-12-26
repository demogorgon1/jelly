cmake_minimum_required(VERSION 3.15)

target_include_directories(jelly PUBLIC  
    $<INSTALL_INTERFACE:include>    
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

file(GLOB H_FILES "jelly/*.h")

target_sources(jelly PRIVATE ${H_FILES})

install(DIRECTORY jelly DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
