//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_PLATFORM_GENERIC_HPP__
#define NETSER_PLATFORM_GENERIC_HPP__

#include <cstddef>
#include <netser/mem_access.hpp>

#if defined(_MSC_VER)
#define NETSER_FORCE_INLINE __forceinline
#elif defined(__GNUC__)
#define NETSER_FORCE_INLINE inline __attribute__((always_inline))
#endif

namespace netser
{

    namespace detail
    {
        template <size_t Byte, typename T>
        NETSER_FORCE_INLINE T mirror_byte(T val)
        {
            return ((val >> (Byte * 8)) & 0xff) << ((sizeof(T) - Byte - 1) * 8);
        }
    } // namespace detail

    template <typename T>
    struct platform_generic_wrapper
    {
        platform_generic_wrapper(T Val) : val(Val)
        {
        }

        T val;
    };

    NETSER_FORCE_INLINE unsigned char byte_swap(platform_generic_wrapper<unsigned char> val)
    {
        return val.val;
    }

    NETSER_FORCE_INLINE unsigned short byte_swap(platform_generic_wrapper<unsigned short> val)
    {
#if defined(__GNUC__)
        return __builtin_bswap16(val.val);
#else
        using namespace detail;
        auto v = val.val;
        return (v >> 8) | (v << 8);
#endif
        // return mirror_byte<0>(v) | mirror_byte<1>(v);
    }

    NETSER_FORCE_INLINE unsigned int byte_swap(platform_generic_wrapper<unsigned int> val)
    {
#ifdef __GNUC__
        return __builtin_bswap32(val.val);
#else
        using namespace detail;
        auto v = val.val;
        return mirror_byte<0>(v) | mirror_byte<1>(v) | mirror_byte<2>(v) | mirror_byte<3>(v);
#endif
    }

    NETSER_FORCE_INLINE unsigned long long byte_swap(platform_generic_wrapper<unsigned long long> val)
    {
#ifdef __GNUC__
        return __builtin_bswap64(val.val);
#else
        using namespace detail;
        auto v = val.val;
        return mirror_byte<0>(v) | mirror_byte<1>(v) | mirror_byte<2>(v) | mirror_byte<3>(v) | mirror_byte<4>(v) | mirror_byte<5>(v)
               | mirror_byte<6>(v) | mirror_byte<7>(v);
#endif
    }

} // namespace netser

#endif
