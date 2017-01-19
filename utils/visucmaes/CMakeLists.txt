# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON) # use this if you have CMake 3.x instead of the following

# Find the QtWidgets library
find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)

add_definitions("--std=c++17")

set(SRCF
  main.cpp
  visucmaes.cpp
  plot.cpp
  plotdocument.cpp
  qcustomplot.cpp
  icons.qrc
)

# Tell CMake to create the helloworld executable
add_executable(visucmaes ${SRCF})

# Use the Widgets module from Qt 5.
target_link_libraries(visucmaes Qt5::Widgets Qt5::PrintSupport)

# Install the executable
install(TARGETS visucmaes DESTINATION bin)
