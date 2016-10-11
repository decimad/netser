
include_directories( ../include )
include_directories( ../netser-devel )

set( NETSER_HEADERS
    ../include/netser/aligned_ptr.hpp
    ../include/netser/fwd.hpp
    ../include/netser/integer.hpp
    ../include/netser/layout.hpp
    ../include/netser/layout_fields.hpp
    ../include/netser/mapping.hpp
    ../include/netser/mem_access.hpp
    ../include/netser/platform_generic.hpp
    ../include/netser/remainder.hpp
    ../include/netser/static_array.hpp
    ../include/netser/type_list.hpp
    ../include/netser/utility.hpp
    ../include/netser/zip_iterator.hpp
    ../include/netser/zipped.hpp
    ../include/netser/netser.hpp
)

source_group(netser FILES ${NETSER_HEADERS})

set( NETSER_DEVEL_FILES
    main.cpp
    merge_sort.hpp
)

source_group(source FILES ${NETSER_DEVEL_FILES})

add_executable(netser-devel main.cpp ${NETSER_HEADERS} ${NETSER_DEVEL_FILES})

