
add_custom_target(pede COMMAND "$(MAKE)" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_custom_command(TARGET pede POST_BUILD COMMAND "cp" "${CMAKE_CURRENT_SOURCE_DIR}/pede" "${CMAKE_BINARY_DIR}")
add_library(mille Mille.cc)
add_dependencies(mille pede)
add_custom_command(TARGET mille PRE_BUILD COMMAND "cp" "${CMAKE_CURRENT_SOURCE_DIR}/Mille.h" "${CMAKE_BINARY_DIR}")
