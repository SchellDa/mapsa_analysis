
project(mapsa_analyses)
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(ROOT REQUIRED)
get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()
add_definitions("--std=c++11" ${ROOT_DEFINITIONS})
link_libraries(core ${Boost_LIBRARIES} ${ROOT_LIBRARIES})
include_directories("${CMAKE_SOURCE_DIR}/core/include" ${Boost_INCLUDE_DIR} ${ROOT_INCLUDE_DIR})

add_executable(analyses main.cpp efficiency_naive.cpp test.cpp)
