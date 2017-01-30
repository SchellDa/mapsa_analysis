# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON) # use this if you have CMake 3.x instead of the following

# Find the QtWidgets library
find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt53DCore REQUIRED)
find_package(Qt53DExtras REQUIRED)
find_package(OpenGL REQUIRED)

add_definitions("--std=c++17")
include_directories(${OPENGL_INCLUDE_DIR})

set(SRCF
  main.cpp
  visucmaes.cpp
  plot.cpp
  plotdocument.cpp
  curvesettings.cpp
  plotsettings.cpp
  globalsettings.cpp
  database.cpp
  qcustomplot.cpp
  view3d.cpp
  viewport.cpp
  icons.qrc
)

# Tell CMake to create the helloworld executable
add_executable(visucmaes ${SRCF})

# Use the Widgets module from Qt 5.
target_link_libraries(visucmaes Qt5::Widgets Qt5::PrintSupport Qt5::3DCore Qt5::3DExtras ${OPENGL_LIBRARY} -lsqlite3)

# Install the executable
install(TARGETS visucmaes DESTINATION bin)
