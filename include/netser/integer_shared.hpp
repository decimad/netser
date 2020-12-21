//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_INTEGER_SHARED_HPP__
#define NETSER_INTEGER_SHARED_HPP__

#include <netser/mem_access.hpp>
#include <netser/zip_iterator.hpp>
#include <type_traits>
#include <meta/util.hpp>

namespace netser
{
    using std::size_t;

    template <bool Signed, size_t Bits, byte_order Endianess>
    struct int_;

    template <typename Type>
    struct is_integer : public std::false_type
    {
    };

    template <bool Signed, size_t Bits, byte_order ByteOrder>
    struct is_integer<int_<Signed, Bits, ByteOrder>> : public std::true_type
    {
    };

    // true iff T is an instance of netser::int_
    template <typename T>
    constexpr bool is_integer_v = is_integer<T>::value;

    namespace detail
    {

        template <typename T>
        struct decode_integer
        {
            static constexpr bool value = false;
        };

        template <bool Signed, size_t Bits, byte_order Endianess>
        struct decode_integer<int_<Signed, Bits, Endianess>>
        {
            static constexpr bool is_signed = Signed;
            static constexpr size_t bits = Bits;
            static constexpr byte_order endianess = Endianess;

            static constexpr bool value = true;
        };

        template <size_t Bits>
        using auto_stage_type_t
            = std::conditional_t<(Bits <= 8), unsigned char,
                                 std::conditional_t<(Bits <= 16), unsigned short,
                                                    std::conditional_t<(Bits <= 32), unsigned int,
                                                                       std::conditional_t<(Bits <= 64), unsigned long long, meta::error_type>>>>;

    } // namespace detail

} // namespace netser

#endif
