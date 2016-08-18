project(mapsa_analysis_core)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SOURCE ${SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/src/cfgparse.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/mpastreamreader.cpp
)

add_definitions("-Wall")
add_definitions("--std=c++11")
if(CMAKE_COMPILER_IS_GNUCC)
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
		add_definitions("-DNO_IOSTREAM_MOVE")
	endif()
endif()

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
 link_libraries(gtest core)
 add_executable(cfgparser_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/cfg_parser_tests.cpp)
 add_executable(mpareader_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/mpa_stream_reader_tests.cpp)
 add_test(cfgparser cfgparser_test)
 add_test(mpareader mpareader_test)
endif()
