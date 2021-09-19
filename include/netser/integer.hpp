//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_INTEGER_HPP__
#define NETSER_INTEGER_HPP__

#include <limits>
#include <random>
#include <type_traits>

#ifdef NETSER_DEBUG_CONSOLE
#include <iostream>
#endif

#include <netser/mem_access.hpp>
#include <netser/field.hpp>

namespace netser
{

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

    // Integer default mapping (If sub-byte, it must not span byte borders, if multi-byte, it must be byte-aligned)
    template <bool Signed, size_t Bits, byte_order Endianess>
    struct int_ : public detail::simple_field_layout_mixin<int_<Signed, Bits, Endianess>>
    {

        //
        // Basic Layout interface
        //
        static constexpr size_t count = 1;
        static constexpr size_t size = Bits;
        static constexpr byte_order endianess = Endianess;

        // iterator2 {
        static constexpr size_t num_children = 0;
        // iterator2 }

        template <size_t Index, size_t BitOffset>
        struct get_field
        {
            static_assert(Index == 0, "Error!");
            static constexpr size_t offset = BitOffset;
            using type = int_;
        };

        //
        // Basic leaf interface
        //
        using stage_type = detail::auto_stage_type_t<Bits>;
        using integral_type = std::conditional_t<Signed, std::make_signed_t<stage_type>, std::make_unsigned_t<stage_type>>;

        // min, max utility
        //
        //
        static constexpr stage_type min()
        {
            return std::numeric_limits<stage_type>::min() & bit_mask<stage_type>(Bits);
        }

        static constexpr stage_type max()
        {
            return std::numeric_limits<stage_type>::max() & bit_mask<stage_type>(Bits - (Signed ? 1 : 0));
        }

      private:
        // defined in integer_read.hpp
        template <typename LayoutIterator>
        static constexpr NETSER_FORCE_INLINE integral_type read(LayoutIterator it);

      public:
        // defined in integer_read.hpp
        template <typename ZipIterator>
        static constexpr NETSER_FORCE_INLINE auto read_span(ZipIterator it);

        // defined int integer_write.hpp
        template <typename ZipIterator>
        static constexpr NETSER_FORCE_INLINE auto write_span(ZipIterator it);

        //
        template <typename MappingIterator, typename Generator>
        static NETSER_FORCE_INLINE void fill_random(MappingIterator it, Generator &&generator)
        {
            std::uniform_int_distribution<std::conditional_t<std::is_same<stage_type, unsigned char>::value, unsigned short, stage_type>>
                dis(min(), max());

            using lhs_type = std::remove_reference_t<decltype(*it)>;

            *it = static_cast<lhs_type>(static_cast<stage_type>(dis(generator)));
        }

        template <typename DestType, typename T>
        static constexpr DestType extract(T val)
        {
            return static_cast<DestType>(val) & bit_mask<DestType>(size);
        }
    };

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


    template <bool Signed, size_t Bits, byte_order ByteOrder>
    template <typename LayoutIterator>
    constexpr typename int_<Signed, Bits, ByteOrder>::integral_type
    int_<Signed, Bits, ByteOrder>::read(LayoutIterator layit)
    {
#ifdef NETSER_DEBUG_CONSOLE
        std::cout << "Generating memory access List (";
        std::cout << "Field size: " << meta::dereference_t<typename LayoutIterator::range>::size
                  << " bits. Ptr-Alignment: " << layit.get_access_alignment(0) << ")... ";
#endif
        using accesses = detail::generate_partial_memory_access_list_t<LayoutIterator, unsigned int>;

#ifdef NETSER_DEBUG_CONSOLE
        detail::describe_generate_partial_memory_access_list_t<LayoutIterator, unsigned int>();
        std::cout << "Got " << list_size_v<accesses> << " reads.\n";
#endif
        return static_cast<integral_type>(detail::template run_access_list<accesses>::template run<stage_type>(layit.get()));
    }

    template <bool Signed, size_t Bits, byte_order ByteOrder>
    template <typename ZipIterator>
    constexpr auto int_<Signed, Bits, ByteOrder>::read_span(ZipIterator it)
    {
        static_assert(!std::is_const<decltype(it.mapping())>::value, "Error!");
        static_assert(!std::is_const<decltype(*it.mapping())>::value, "Error!");

#ifdef NETSER_DEBUG_CONSOLE
        std::cout << "reading integer span:\n";
#endif
        *it.mapping() = read(it.layout());

#ifdef NETSER_DEBUG_CONSOLE
        std::cout << "read span complete.\n";
#endif
        return ++it;
    }

    template <bool Signed, size_t Bits, byte_order ByteOrder>
    template <typename ZipIterator>
    NETSER_FORCE_INLINE constexpr auto int_<Signed, Bits, ByteOrder>::write_span(ZipIterator it)
    {
        // The new implementation takes two steps to calculate the next write(s).
        // 1. Look ahead the iterator sequence to discover what access type to use
        // 2. Complete this write
        // 3. If any field was not committed completely, restart at step 1.

        // This happens in order to decrease the lookahead depth and the amount of data which is taken along the iterations.
        // Involved type names get very longish very quickly, so this implementation tries to keep them as short as possible.

        return detail::write_integer_algorithm::write_integer<>(it);
    }

    template <size_t Size>
    using net_uint = int_<false, Size, byte_order::be>;

    template <size_t Size>
    using net_int = int_<true, Size, byte_order::be>;

    using net_int8 = net_int<8>;
    using net_int16 = net_int<16>;
    using net_int32 = net_int<32>;

    using net_uint8 = net_uint<8>;
    using net_uint16 = net_uint<16>;
    using net_uint32 = net_uint<32>;

} // namespace netser

#endif
