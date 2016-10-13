//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_MEM_ACCESS_HPP__
#define NETSER_MEM_ACCESS_HPP__

#include <type_traits>
#include "netser_config.hpp"

namespace netser {

    namespace detail {

        template< bool Predicate >
        struct byte_swap_switch_struct
        {
            template< typename T >
            static NETSER_FORCE_INLINE T swap(T val) {
                return byte_swap(byte_swap_wrapper<T>(val));
            }
        };

        template< >
        struct byte_swap_switch_struct< false >
        {
            template< typename T >
            static NETSER_FORCE_INLINE T swap(T val) {
                return val;
            }
        };

    }

    template< bool Predicate, typename T >
    T NETSER_FORCE_INLINE conditional_swap(T val)
    {
        return detail::byte_swap_switch_struct<Predicate>::swap(val);
    }

    template< typename AtomicMemoryAccess, int Offset >
    struct placed_atomic_memory_access;

    // atomic_memory_access
    //
    //
    template< typename Type, size_t Size, size_t Alignment, typename Endianess >
    struct atomic_memory_access
    {
        using type = Type;
        using endianess = Type;
        static constexpr size_t size = Size;
        static constexpr size_t alignment = Alignment;

        template< int Offset, typename AlignedPtr >
        static type read(AlignedPtr src)
        {
            static_assert(Offset % 8 == 0, "Bad Offset!");
            return src.template dereference<type, Offset/8>();
        }

        template< int Offset, typename AlignedPtr >
        static type write(AlignedPtr dest, type value)
        {
            static_assert(Offset % 8 == 0, "Bad Offset!");
            dest.template dereference<type, Offset/8>() = value;
        }

        template< int Offset >
        using placed = placed_atomic_memory_access< atomic_memory_access, Offset >;

        template< int Offset, typename AlignedPtr >
        using aligned_range = bit_range<
            AlignedPtr::align_down(Offset/8, Alignment)*8,
            AlignedPtr::align_down(Offset/8, Alignment)*8 + int(Size*8)
        >;
    };

    namespace detail {

        namespace impl {
            template< typename Type, size_t Size, size_t Alignment, typename Endianess >
            std::true_type  is_atomic_memory_access_func(atomic_memory_access<Type, Size, Alignment, Endianess>);

            template< typename T >
            std::false_type is_atomic_memory_access_func(T);
        }

        template< typename T >
        using is_atomic_memory_access_t = std::remove_all_extents_t<decltype(impl::is_atomic_memory_access_func(std::declval<T>()))>;

        namespace impl {
            template< typename Access, size_t Offset >
            std::true_type  is_placed_atomic_memory_access_func(placed_atomic_memory_access<Access, Offset>);

            template< typename T >
            std::false_type is_placed_atomic_memory_access_func(T);
        }

        template< typename T >
        using is_placed_atomic_memory_access_t = std::remove_all_extents_t<decltype(impl::is_placed_atomic_memory_access_func(std::declval<T>()))>;

    }

    template< typename AtomicMemoryAccess, int Offset >
    struct placed_atomic_memory_access : public AtomicMemoryAccess {
        using atomic_access = AtomicMemoryAccess;
        static_assert(detail::is_atomic_memory_access_t<AtomicMemoryAccess>::value, "Bad Argument!");
        static_assert(Offset % 8 == 0, "Access is not byte-aligned!");

        static constexpr int bits_offset = Offset;
        static constexpr int byte_offset = Offset / 8;

        using bits_range = bit_range< bits_offset, bits_offset + atomic_access::size * 8 >;
        using byte_range = bit_range< byte_offset, byte_offset + atomic_access::size >;

#ifdef NETSER_DEBUG_CONSOLE
        static void describe()
        {
            std::cout << "Writing " << atomic_access::size << " bytes.\n";
        }
#endif

        template< typename AlignedPtr >
        static typename atomic_access::type read(AlignedPtr src)
        {
            static_assert(AlignedPtr::get_access_alignment(byte_offset) % atomic_access::alignment == 0, "Bad Offset!");
            return src.template dereference<atomic_access::type, byte_offset>();
        }

        template< typename AlignedPtr >
        static void write(AlignedPtr dest, typename atomic_access::type value)
        {
            static_assert(AlignedPtr::get_access_alignment(byte_offset) % atomic_access::alignment == 0, "Offset does not meet alignment requirements!");
            dest.template dereference<atomic_access::type, byte_offset>() = value;
        }
    };

    struct le {};
    struct be {};

    using little_endian = le;
    using big_endian    = be;

    template< typename... >
    struct memory_access_list {};

    namespace detail {

        namespace impl {

            // ===========================
            // filter_memory_access_list
            // filters available memory accesses for buffer pointer alignment
            //
            template< size_t BufferAlignment, typename MemoryAccessList, typename ResultAccessList = type_list< > >
            struct filter_memory_access_list;

            template< size_t BufferAlignment, typename... ResultAccesses >
            struct filter_memory_access_list< BufferAlignment, type_list< >, type_list< ResultAccesses... > >
            {
                using result = type_list< ResultAccesses... >;
            };

            template< size_t BufferAlignment, typename SourceAccess0, typename... SourceAccesses, typename... ResultAccesses >
            struct filter_memory_access_list< BufferAlignment, type_list< SourceAccess0, SourceAccesses... >, type_list< ResultAccesses... > >
            {
                using result = typename filter_memory_access_list<
                    BufferAlignment,
                    type_list< SourceAccesses... >,
                    std::conditional_t<
                        ((BufferAlignment % SourceAccess0::alignment) == 0),
                        type_list< ResultAccesses..., SourceAccess0 >,
                        type_list< ResultAccesses... >
                    >
                >::result;
            };

        }

    }

    // =================================
    // platform_filtered_memory_accesses
    // list of memory accesses possible given a certain buffer alignment
    template< size_t Alignment >
    using platform_filtered_memory_accesses = typename detail::impl::filter_memory_access_list< Alignment, platform_memory_accesses >::result;

    namespace detail {

        struct empty_access {
            using intersection_range = bit_range< 0, 0 >;
        };

        //
        // placed_memory_access
        //
        template< typename Access, int Offset, typename AlignedPtr >
        struct placed_memory_access
        {
//            static_assert( Offset % 8 == 0, "Unaligned offset!" );
            static constexpr int offset = Offset;
            using access = Access;
            using type   = typename access::type;
            using range  = typename access::template aligned_range<Offset, AlignedPtr>;

            using endianess = typename Access::endianess;

            template< typename AAlignedPtr >
            static type read(AAlignedPtr src)
            {
                return access::template read<range::begin>(src);
            }

            template< typename T >
            static void write(const void* src, T value)
            {
                access::template write<range::begin>(src, value);
            }
        };

    }

}

#endif
