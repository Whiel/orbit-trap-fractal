cmake_minimum_required( VERSION 2.6.0 )

# Project name
project( julia )

# Set extra compiler flags
if( UNIX )
  set( CMAKE_CXX_FLAGS "-Wall -Werror=return-type -g -pthread" )
endif( UNIX )

# Add sources
aux_source_directory( . julia_src )

# Where to find headers
include_directories( .  julia_src )
include_directories( .. julia_src )

# Required libraries are added to this variable
set( requiredLibs )

# Where to look for extra modules used to find libraries
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/cmake_modules/" )

# OpenGL (GL, GLU, GLEXT)
find_package( OpenGL REQUIRED )
include_directories( SYSTEM ${OPENGL_INCLUDE_DIR} )
set( requiredLibs ${requiredLibs} ${OPENGL_LIBRARIES} )

# GLEW
if( WIN32 )
  find_library( GLEW_LIBRARIES glew32.lib )
  set( requiredLibs ${requiredLibs} ${GLEW_LIBRARIES} )
endif( WIN32 )
if( UNIX )
  find_package( GLEW REQUIRED )
  include_directories( ${GLEW_INCLUDE_DIR} )
  set( requiredLibs ${requiredLibs} ${GLEW_LIBRARY} )
endif( UNIX )

# GLM
#include_directories(SYSTEM ../libs/glm )

# GLFW3
find_package (GLFW3 REQUIRED glfw3)
include_directories (SYSTEM ${GLFW3_INCLUDE_DIR})
set( requiredLibs ${requiredLibs} ${GLFW3_LIBRARY})

# lodepng
#aux_source_directory( ../libs/lodepng julia_src )
#include_directories(SYSTEM ../libs/lodepng )

# AntTweakBar
#include_directories(SYSTEM ../libs/AntTweakBar/include )
#link_directories( ../libs/AntTweakBar/lib )
#set( requiredLibs ${requiredLibs} AntTweakBar )

# Create build files for executable
add_executable( julia ${julia_src} )

# Link against libraries
target_link_libraries( julia ${requiredLibs} ${optionalLibs} )

# Build type
set( CMAKE_BUILD_TYPE Debug )
