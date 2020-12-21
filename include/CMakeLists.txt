# dependencies: meta

add_library(netser.iface INTERFACE)
target_include_directories(netser.iface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(netser.iface INTERFACE meta.iface)
