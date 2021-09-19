
add_executable(netser-devel baby_test.cpp ptp_announce_test.cpp)

target_include_directories(netser-devel PRIVATE ../include )
target_include_directories(netser-devel PRIVATE ../netser-devel )

target_include_directories(netser-devel PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../meta)

target_compile_options(netser-devel PRIVATE -std=gnu++20)
target_compile_options(netser-devel
    PRIVATE
        -mcpu=cortex-m7
        -mthumb
        -mfloat-abi=hard
        -mfpu=fpv5-d16
)

target_link_options(netser-devel PRIVATE
    -mcpu=cortex-m7
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv5-d16
)

add_custom_command(TARGET netser-devel POST_BUILD
    COMMAND arm-none-eabi-objdump -D $<TARGET_FILE:netser-devel> > disassembly.txt
)