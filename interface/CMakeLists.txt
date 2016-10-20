
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(interface
	"${CMAKE_CURRENT_SOURCE_DIR}/src/DataFormats.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/Dict.cpp"
)
