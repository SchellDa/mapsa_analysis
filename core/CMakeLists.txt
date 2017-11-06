project(mapsa_analysis_core)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/root_dict.cpp ${CMAKE_BINARY_DIR}/root_dict.h
	COMMAND ${ROOT_CINT_EXECUTABLE} -f ${CMAKE_BINARY_DIR}/root_dict.cpp -c
	"${CMAKE_CURRENT_SOURCE_DIR}/include/datastructures.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/LinkDef.h"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/datastructures.h" 
	"${CMAKE_CURRENT_SOURCE_DIR}/include/LinkDef.h"
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SOURCE ${SOURCE}
	${CMAKE_CURRENT_SOURCE_DIR}/src/core.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/cfgparse.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/mpastreamreader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/mpamemorystreamreader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/cbcstreamreader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/trackstreamreader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/analysis.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/trackanalysis.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/mergedanalysis.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/quickrunlistreader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/runlistreader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/aligner.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/functions.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/mpatransform.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/histogramfit.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/datastructures.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/triplet.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/triplettrack.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/mpahitgenerator.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/alibavaanalysis.cpp
	${CMAKE_BINARY_DIR}/root_dict.cpp
)

add_definitions("-Wall")
add_definitions("--std=c++11")
add_definitions("-O3")
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

add_definitions("-fPIC")
add_library(core STATIC ${SOURCE})

if(${ENABLE_CBC_ANALYSIS})
target_link_libraries(core interface)
endif(${ENABLE_CBC_ANALYSIS})

if(BUILD_TESTING)
 add_definitions("-Wno-unused-variable")
 include_directories(${GOOGLETEST_INCLUDE_DIR})
 link_libraries(gtest core)
 add_executable(cfgparser_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/cfg_parser_tests.cpp)
 add_executable(mpareader_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/mpa_stream_reader_tests.cpp)
 add_executable(trackreader_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/track_stream_reader_tests.cpp)
 add_executable(mpatransform_test ${CMAKE_CURRENT_SOURCE_DIR}/tests/mpatransform_test.cpp)
 add_test(cfgparser cfgparser_test)
 add_test(mpareader mpareader_test)
 add_test(trackreader trackreader_test)
endif()
