#
# add_gtest_test(<target> <sources>...)
#
#  Adds a Google Test based test executable, <target>, built from <sources> and
#  adds the test so that CTest will run it. Both the executable and the test
#  will be named <target>.
#
function(add_gtest_test target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} ${GTEST_BOTH_LIBRARIES})
    target_include_directories(${target} PRIVATE ../include ../../meta)

    set_target_properties(${target} PROPERTIES FOLDER tests)
    add_test(${target} ${target})
    set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endfunction()

add_gtest_test( integer-read  integer-read.cpp )
add_gtest_test( integer-write integer-write.cpp )
add_gtest_test( zipped zipped.cpp )