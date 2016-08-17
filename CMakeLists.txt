cmake_minimum_required(VERSION 2.8.11)
project(mapsaAnalysis)

add_definitions("--std=c++11")
add_subdirectory(src)
configure_file(config.in ${CMAKE_BINARY_DIR}/config.h)
include_directories(${CMAKE_BINARY_DIR})

add_executable(mapsa_analysis ${SOURCE})
# target_link_libraries(mapsaAnalysis )
