//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_INTEGER_HPP__
#define NETSER_INTEGER_HPP__

#ifdef NETSER_DEBUG_CONSOLE
#include <iostream>
#endif
#include <netser/layout.hpp>
#include <netser/mem_access.hpp>
#include <netser/integer_shared.hpp>

namespace netser {

    // Integer default mapping (If sub-byte, it must not span byte borders, if multi-byte, it must be byte-aligned)
    template< bool Signed, size_t Bits, typename ByteOrder >
    struct int_ : public detail::simple_field_layout_mixin< int_<Signed, Bits, ByteOrder> > {

        //
        // Basic Layout interface
        //
        static constexpr size_t count = 1;
        static constexpr size_t size = Bits;

        using endianess = ByteOrder;

        template< size_t Index, size_t BitOffset >
        struct get_field {
            static_assert(Index == 0, "Error!");
            static constexpr size_t offset = BitOffset;
            using type = int_;
        };

        //
        // Basic leaf interface
        //
        using stage_type    = detail::auto_stage_type_t< Bits >;
        using integral_type = std::conditional_t< Signed, std::make_signed_t< stage_type >, std::make_unsigned_t< stage_type > >;

        // defined in integer_read.hpp
        template< typename LayoutIterator >
        static constexpr NETSER_FORCE_INLINE integral_type read( LayoutIterator it );

        // defined in integer_read.hpp
        template< typename ZipIterator >
        static constexpr NETSER_FORCE_INLINE auto read_span( ZipIterator it );

        // defined int integer_write.hpp
        template< typename ZipIterator >
        static constexpr NETSER_FORCE_INLINE auto write_span( ZipIterator it );

        template< typename DestType, typename T >
        static constexpr DestType extract(T val) {
            return static_cast<DestType>(val) & bit_mask<DestType>(size);
        }

    };

    template< size_t Size >
    using net_uint = int_< false, Size, be >;

    template< size_t Size >
    using net_int = int_< true, Size, be >;

    using net_int8   = net_int<8>;
    using net_int16  = net_int<16>;
    using net_int32  = net_int<32>;

    using net_uint8  = net_uint<8>;
    using net_uint16 = net_uint<16>;
    using net_uint32 = net_uint<32>;

}

#include <netser/integer_read.hpp>
#include <netser/integer_write.hpp>

#endif
