cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
cmake_policy(SET CMP0074 NEW)
cmake_policy(SET CMP0091 NEW)

project(netser)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(GTEST_ROOT "E:/projects/3rdparty-build/googletest-release-1.10.0/out/install/x64-Release")

if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  message("netser: standalone build")
  add_subdirectory( netser-devel )
else()
  message("Detected nested build.")
  add_subdirectory( include )
endif()