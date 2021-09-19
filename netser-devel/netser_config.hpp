//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

// #ifdef _MSC_VER
// #define NETSER_FORCE_INLINE __forceinline
// #else
// #define NETSER_FORCE_INLINE __attribute__((always_inline))
// #endif

#include <netser/platform_toolkit.hpp>
#include <netser/utility.hpp>
#include <meta/tlist.hpp>

namespace netser
{

#ifdef _MSC_VER
    using uint64 = unsigned __int64;
    using int64 = __int64;
#else
    using uint64 = unsigned long long;
    using int64 = long long;
#endif

    template <typename T>
    using byte_swap_wrapper = platform_generic_wrapper<T>;

    using platform_memory_accesses = meta::tlist<
        atomic_memory_access<unsigned char, 1, 1, byte_order::little_endian>,
        atomic_memory_access<unsigned short, 2, 2, byte_order::little_endian>,
        atomic_memory_access<unsigned int, 4, 4, byte_order::little_endian>,
        atomic_memory_access<uint64, 8, 8, byte_order::little_endian>
    >;

} // namespace netser
