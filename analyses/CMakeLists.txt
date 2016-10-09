
project(mapsa_analyses)
find_package(Boost REQUIRED COMPONENTS program_options)
#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()
add_definitions("--std=c++11")
link_libraries(core ${Boost_LIBRARIES})
include_directories("${CMAKE_SOURCE_DIR}/core/include" ${Boost_INCLUDE_DIR})

add_executable(analyses main.cpp test.cpp efficiency_track.cpp data_skip.cpp clusterize.cpp z_align_test.cpp)
