cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
cmake_policy(SET CMP0074 NEW)

project("netser")

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(GTEST_ROOT "E:/projects/3rdparty-build/googletest-release-1.10.0/out/install/x64-Release")

add_compile_options(/std:c++latest)


if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  macro(SOURCE_GROUP_BY_FOLDER base sources)
    source_group("" FILES ${sources})
    set(SOURCE_GROUP_DELIMITER "/")
    set(last_dir "")
    set(files "")
    foreach(file ${sources})
      get_filename_component(absolute ${file} ABSOLUTE)
      file(RELATIVE_PATH relative_file "${base}" ${absolute})
      message( "Rel file: ${relative_file}" )
      get_filename_component(dir "${relative_file}" PATH)
      if (NOT "${dir}" STREQUAL "${last_dir}")
        if (files)
          source_group("${last_dir}" FILES ${files})
        endif()
        set(files "")
      endif ()
      set(files ${files} ${file})
      set(last_dir "${dir}")
    endforeach()
    if (files)
      source_group("${last_dir}" FILES ${files})
    endif (files)
  endmacro()

  add_subdirectory( netser-devel )

  if (MSVC)
      # force this option to ON so that Google Test will use /MD instead of /MT
      # /MD is now the default for Visual Studio, so it should be our default, too
      option(gtest_force_shared_crt
            "Use shared (DLL) run-time lib even when Google Test is built as static lib."
            ON)
  elseif (APPLE)
      add_definitions(-DGTEST_USE_OWN_TR1_TUPLE=1)
  endif()

  message("Detected standalone build.")

  enable_testing()
  find_package(GTest REQUIRED)
  include_directories(SYSTEM ${GTEST_INCLUDE_DIRS})

  add_subdirectory( tests )
else()

  message("Detected nested build.")

  add_subdirectory( include )
endif()