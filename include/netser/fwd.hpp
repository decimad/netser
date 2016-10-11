//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_FWD_HPP___
#define NETSER_FWD_HPP___

#include <cstddef>
using std::size_t;

namespace netser {

    template< typename Type, size_t Size, size_t Alignment, typename Endianess >
    struct atomic_memory_access;

    template< typename... >
    struct memory_access_list;

    struct le;
    struct be;

    using little_endian = le;
    using big_endian    = be;

}

#endif
