cmake_minimum_required(VERSION 3.13)
project(libambient VERSION 1.1)

#Disable in source-builds
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

set(LIB_AMBIENT_BIN "${CMAKE_BINARY_DIR}../bin/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${LIB_AMBIENT_BIN})

# --------------------  Sources --------------------
file(GLOB_RECURSE CXX_SOURCES RELATIVE  ${CMAKE_SOURCE_DIR} "src/*.cpp")
file(GLOB_RECURSE C_SOURCES RELATIVE    ${CMAKE_SOURCE_DIR} "src/*.c")
include_directories("include/")

add_library(libambient SHARED ${CXX_SOURCES}  ${C_SOURCES})
