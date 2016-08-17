project(mapsa_analysis_core)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SOURCE ${SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/src/cfgparse.cpp
)

add_definitions("-Wall")
add_definitions("--std=c++11")

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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/coreconfig.in ${CMAKE_BINARY_DIR}/coreconfig.h)
include_directories(${CMAKE_BINARY_DIR})

add_library(core STATIC ${SOURCE})
if(BUILD_TESTING)
 include_directories(${GOOGLETEST_INCLUDE_DIR})
 add_executable(coreTests
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/cfg_parser_tests.cpp
 )
 target_link_libraries(coreTests gtest core)
add_test(coreTests coreTests)
endif()
