
cmake_minimum_required(VERSION 3.10)
project(FluidSimulation)

set(CMAKE_CXX_STANDARD 17)

# Add Homebrew paths for Apple Silicon
set(CMAKE_PREFIX_PATH /opt/homebrew ${CMAKE_PREFIX_PATH})

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find GLFW
find_package(glfw3 REQUIRED)

# Find GLM
find_package(glm REQUIRED)

# Manually set GLEW paths for macOS with Homebrew
set(GLEW_INCLUDE_DIRS /opt/homebrew/include)
set(GLEW_LIBRARIES /opt/homebrew/lib/libGLEW.dylib)
include_directories(${GLEW_INCLUDE_DIRS})
message(STATUS "Using GLEW: ${GLEW_LIBRARIES}")

# Include directories
include_directories(${OPENGL_INCLUDE_DIRS} ${GLFW3_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS})

# Source files
add_executable(FluidSimulation 
    main.cpp 
    fluid.cpp 
    render.cpp 
    utils.cpp
)

# Link libraries
target_link_libraries(FluidSimulation ${OPENGL_LIBRARIES} glfw ${GLEW_LIBRARIES})
