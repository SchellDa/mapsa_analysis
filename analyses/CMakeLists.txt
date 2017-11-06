
project(mapsa_analyses)
find_package(Boost REQUIRED COMPONENTS program_options)

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

add_definitions("--std=c++11")
add_definitions("-O3")
link_libraries(core ${Boost_LIBRARIES})
include_directories("${CMAKE_SOURCE_DIR}/core/include" ${Boost_INCLUDE_DIR})

add_library(AnalysisClasses SHARED test.cpp efficiency_track.cpp data_skip.cpp 
			    	   clusterize.cpp mpa_align.cpp 
				   strip_efficiency.cpp strip_align.cpp 
				   mpa_efficiency.cpp mpa_minuit_align.cpp 
				   refprealign.cpp gblalign.cpp 
				   mpatripletefficiency.cpp mpa_cluster_test.cpp
				   alibava_dqm.cpp alibavaefficiency.cpp
				   ) # mpa_cmaes_align.cpp 
add_executable(analyses main.cpp)
target_link_libraries(analyses AnalysisClasses)
