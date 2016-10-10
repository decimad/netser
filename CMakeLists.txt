cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)

project("netser")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

MACRO(SOURCE_GROUP_BY_FOLDER base sources)
  source_group("" FILES ${sources})
  SET(SOURCE_GROUP_DELIMITER "/")
  SET(last_dir "")
  SET(files "")
  FOREACH(file ${sources})
    get_filename_component(absolute ${file} ABSOLUTE)
    file(RELATIVE_PATH relative_file "${base}" ${absolute})
    message( "Rel file: ${relative_file}" )
    GET_FILENAME_COMPONENT(dir "${relative_file}" PATH)
    IF (NOT "${dir}" STREQUAL "${last_dir}")
      IF (files)
        SOURCE_GROUP("${last_dir}" FILES ${files})
      ENDIF (files)
      SET(files "")
    ENDIF (NOT "${dir}" STREQUAL "${last_dir}")
    SET(files ${files} ${file})
    SET(last_dir "${dir}")
  ENDFOREACH(file)
  IF (files)
    SOURCE_GROUP("${last_dir}" FILES ${files})
  ENDIF (files)
ENDMACRO(SOURCE_GROUP_BY_FOLDER)

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


enable_testing()
find_package(GTest REQUIRED)
include_directories(SYSTEM ${GTEST_INCLUDE_DIRS})

#
# add_gmock_test(<target> <sources>...)
#
#  Adds a Google Mock based test executable, <target>, built from <sources> and
#  adds the test so that CTest will run it. Both the executable and the test
#  will be named <target>.
#
function(add_gtest_test target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} ${GTEST_BOTH_LIBRARIES})
    include_directories( include )
    set_target_properties(${target} PROPERTIES FOLDER tests)
    add_test(${target} ${target})
endfunction()

add_gtest_test(integer-test tests/integer-test.cpp)