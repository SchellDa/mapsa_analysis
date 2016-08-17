cmake_minimum_required(VERSION 2.8.11)
project(mapsaAnalysis)

add_definitions("-Wall")
add_definitions("--std=c++11")
add_subdirectory(src)

# Find out version by using 'git describe'
execute_process(COMMAND git describe --tags --long
  RESULT_VARIABLE GIT_DESCRIBE_STATUS
  OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
)
if(${GIT_DESCRIBE_STATUS} EQUAL "0")
  string(STRIP ${GIT_DESCRIBE_VERSION} VERSION_TAG)
else()
  set(VERSION_MAJOR 0)
  set(VERSION_MINOR 1)
endif()

configure_file(config.in ${CMAKE_BINARY_DIR}/config.h)
include_directories(${CMAKE_BINARY_DIR})

add_executable(mapsa_analysis ${SOURCE})
# target_link_libraries(mapsaAnalysis )
