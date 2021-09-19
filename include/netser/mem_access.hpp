//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_MEM_ACCESS_HPP__
#define NETSER_MEM_ACCESS_HPP__

#include <netser/utility.hpp>
#include <netser/fwd.hpp>
#include <type_traits>

namespace netser
{
    enum class byte_order {
        little_endian,
        le = little_endian,
        big_endian,
        be = big_endian
    };

    // atomic_memory_access
    //
    //
    template <typename Type, size_t Size, size_t Alignment, byte_order Endianess>
    struct atomic_memory_access
    {
        using type = Type;
        static constexpr byte_order endianess = Endianess;
        static constexpr size_t size = Size;
        static constexpr size_t alignment = Alignment;

        template <int Offset, typename AlignedPtr> requires(Offset % 8 == 0)
        static type read(AlignedPtr src)
        {
            return src.template dereference<const type, Offset / 8>();
        }

        template <int Offset, typename AlignedPtr> requires(Offset % 8 == 0)
        static type write(AlignedPtr dest, type value)
        {
            dest.template dereference<type, Offset / 8>() = value;
        }

/*
        template <int Offset, typename AlignedPtr>
        using aligned_range = bit_range<AlignedPtr::align_down(Offset / 8, Alignment) * 8,
                                        AlignedPtr::align_down(Offset / 8, Alignment) * 8 + int(Size * 8)>;
*/

        template <int Offset, typename AlignedPtr>
        static constexpr bit_range aligned_range()
        {
            return make_bit_range<AlignedPtr::align_down(Offset / 8, Alignment) * 8,
                                  AlignedPtr::align_down(Offset / 8, Alignment) * 8 + int(Size * 8)>();
        }
    };

    namespace detail
    {

        template <typename T>
        struct is_atomic_memory_access_t
        {
            static constexpr bool value = false;
        };

        template <typename T, size_t Size, size_t Alignment, byte_order Endianess>
        struct is_atomic_memory_access_t<atomic_memory_access<T, Size, Alignment, Endianess>>
        {
            static constexpr bool value = true;
        };

        template <typename T>
        static constexpr bool is_atomic_memory_access_v = is_atomic_memory_access_t<T>::value;

    } // namespace detail

    template <typename AtomicMemoryAccess, int Offset>
    requires(detail::is_atomic_memory_access_v<AtomicMemoryAccess> && (Offset % 8 == 0))
    struct placed_atomic_memory_access : public AtomicMemoryAccess
    {
        using atomic_access = AtomicMemoryAccess;

        static constexpr int bits_offset = Offset;
        static constexpr int byte_offset = Offset / 8;

        static constexpr auto bits_range = make_bit_range<bits_offset, bits_offset + atomic_access::size * 8>();
        static constexpr auto byte_range = make_bit_range<byte_offset, byte_offset + atomic_access::size>();

#ifdef NETSER_DEBUG_CONSOLE
        static void describe()
        {
            std::cout << "Writing " << atomic_access::size << " bytes.\n";
        }
#endif

        template <typename AlignedPtr>
        requires(AlignedPtr::get_access_alignment(byte_offset) % atomic_access::alignment == 0)
        static typename atomic_access::type read(AlignedPtr src)
        {
            return src.template dereference<const typename atomic_access::type, byte_offset>();
        }

        template <typename AlignedPtr>
        requires(AlignedPtr::get_access_alignment(byte_offset) % atomic_access::alignment == 0)
        static void write(AlignedPtr dest, typename atomic_access::type value)
        {
            dest.template dereference<typename atomic_access::type, byte_offset>() = value;
        }
    };

    template <typename...>
    struct memory_access_list
    {
    };

    namespace detail
    {

        template<size_t Alignment>
        struct alignment_matches {
            template<typename Access>
            struct type {
                static constexpr bool value = (Alignment % Access::alignment) == 0;
            };
        };

    } // namespace detail

    namespace detail
    {

        struct empty_access
        {
            static constexpr auto intersection_range = make_bit_range<0, 0>();
        };

        //
        // placed_memory_access
        //
        template <typename Access, int Offset, typename AlignedPtr>
        struct placed_memory_access
        {
            static constexpr int offset = Offset;
            using access = Access;
            using type  = typename access::type;

            static constexpr bit_range range = access::template aligned_range<Offset, AlignedPtr>();

            static constexpr byte_order endianess = Access::endianess;

            template <typename AAlignedPtr>
            static type read(AAlignedPtr src)
            {
                return access::template read<range.begin()>(src);
            }

            template <typename T>
            static void write(const void *src, T value)
            {
                access::template write<range.begin()>(src, value);
            }
        };

    } // namespace detail

} // namespace netser

#endif
