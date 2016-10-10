//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#ifdef _MSC_VER
#define NETSER_FORCE_INLINE __forceinline
#else
#define NETSER_FORCE_INLINE __attribute__((always_inline))
#endif

#include <netser/fwd.hpp>
#include <netser/platform_generic.hpp>
#include <netser/type_list.hpp>

namespace netser {


#ifdef _MSC_VER
	using uint64 = unsigned __int64;
	using int64  = __int64;
#else
	using uint64 = unsigned long long;
	using int64 = long long;
#endif


	template< typename T >
	using byte_swap_wrapper = platform_generic_wrapper<T>;

	using platform_memory_accesses = type_list<
		atomic_memory_access< unsigned char,  1, 1, little_endian >,
		atomic_memory_access< unsigned short, 2, 2, little_endian >,
		atomic_memory_access< unsigned int,   4, 4, little_endian >,
		atomic_memory_access< uint64,         8, 8, little_endian >
	>;

}
