
include_directories( . )

set( NETSER_HEADERS
    netser/aligned_ptr.hpp
    netser/fwd.hpp
    netser/integer.hpp
    netser/layout.hpp
)

add_library(netser INTERFACE)
target_sources(netser INTERFACE ${NETSER_HEADERS})