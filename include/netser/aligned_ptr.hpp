//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_ALIGNED_PTR_HPP__
#define NETSER_ALIGNED_PTR_HPP__

#include <netser/platform.hpp>
#include <netser/remainder.hpp>
#include <meta/tlist.hpp>
#include <netser/utility.hpp>
#include <type_traits>


#ifdef NETSER_DEBUG_CONSOLE
#include <cassert>
#include <iostream>

#endif

namespace netser
{

    using std::size_t;

#ifdef NETSER_DEREFERENCE_LOGGING
    class dereference_logger
    {
      public:
        virtual void log(uintptr_t memory_location, int offset, std::string type_name, size_t type_size, size_t type_alignment) = 0;
    };
#endif

    template <int Begin, int End>
    struct bounded
    {
        static constexpr bool contains(int begin, int end)
        {
            return begin >= Begin && end < End;
        }

        template <int Offset>
        using offset_range = bounded<Begin - Offset, End - Offset>;
    };

    template <int Begin>
    struct lower_bounded
    {
        static constexpr bool contains(int begin, int end)
        {
            (void)end;
            return begin >= Begin;
        }

        template <int Offset>
        using offset_range = lower_bounded<Begin - Offset>;
    };

    // aligned_ptr
    // Class that encapsulates a pointer, its alignment and defect and a valid access range
    //
    template <typename Type, size_t Alignment, size_t Defect = 0, typename OffsetRange = lower_bounded<0>>
    class aligned_ptr
    {
      public:
        using value_type = Type;
        using type = Type *const;

        // Alignment+Defect are actually Alignment+Defect in GCC parlance, which form a residue class
        // from which we can calculate the residue class and thus the alignment at any offset.
        using residue = residue_class<Alignment, Defect>;

        using offset_range = OffsetRange;

      private:
        static constexpr size_t pointer_alignment = residue::alignment();
        static constexpr size_t max_alignment = Alignment;

      public:
        // get_pointer_alignment
        // Returns the true alignment of the stored pointer (before applying the static offset)
        static constexpr size_t get_pointer_alignment()
        {
            return pointer_alignment;
        }

        // get_pointer_alignment
        // Returns the alignment defect wrt. get_max_aligment of the pointer (before applying the static offset)
        static constexpr size_t get_pointer_defect()
        {
            return Defect;
        }

        // get_max_alignment
        // Returns the maximum alignment that can be guaranteed with changing offsets
        static constexpr size_t get_max_alignment()
        {
            return Alignment;
        }

        // get_access_alignment
        // Returns the aligment of an access (after passed offset)
        static constexpr size_t get_access_alignment(int offset_bytes = 0)
        {
            return residue::offset_alignment(offset_bytes);
        }

        // align_down
        // Reduces the given offset so that an access with the offset meets the alignment requirement RequestedAlignment
        static constexpr int align_down(int Offset, size_t RequestedAlignment)
        {
            return residue::align_down(Offset, RequestedAlignment);
        }

        constexpr aligned_ptr(Type *ptr
#ifdef NETSER_DEREFERENCE_LOGGING
                              ,
                              dereference_logger *logger = nullptr
#endif
                              )
            : ptr_(ptr)
#ifdef NETSER_DEREFERENCE_LOGGING
              ,
              logger_(logger)
#endif
        {
#ifdef NETSER_DEBUG_CONSOLE
            assert(reinterpret_cast<uintptr_t>(ptr) % Alignment == Defect);
#endif
        }

        constexpr aligned_ptr(const aligned_ptr &) = default;
        constexpr aligned_ptr(aligned_ptr &&) = default;

        template <int Offset>
        constexpr std::remove_const_t<type> get_offset() const
        {
            static_assert(offset_range::contains(Offset, Offset),
                          "No possible memory access at given location."); // Fixme: Is this over-restrictive?
#ifdef __GNUC__
            // We can hint the alignment+defect, maybe GCC can put it to use
            return reinterpret_cast<type>(__builtin_assume_aligned(reinterpret_cast<copy_constness_t<Type, char> *const>(ptr_) + Offset,
                                                                   residue::template offset_class<Offset>::divisor,
                                                                   residue::template offset_class<Offset>::remainder));
#else
            return reinterpret_cast<type>(reinterpret_cast<copy_constness_t<Type, char> *const>(ptr_) + Offset);
#endif
        }

        constexpr std::remove_const_t<type> get() const
        {
            return get_offset<0>();
        }

        template <typename T, int Offset = 0>
        T &dereference() const
        {
            static_assert(offset_range::contains(Offset, Offset + int(sizeof(T))),
                          "Pointer range does not contain the dereferenced type at given Offset.");
#ifdef NETSER_DEREFERENCE_LOGGING
            if (logger_)
            {
                logger_->log(reinterpret_cast<uintptr_t>(get_offset<0>()), Offset, typeid(T).name(), sizeof(T), alignof(T));
            }
#endif
            return *reinterpret_cast<T *>(reinterpret_cast<copy_constness_t<Type, char> *>(get_offset<Offset>()));
        }

        // static_offset_bits
        // Convenience checked offset.
        // Offset this pointer by a static amount of bits. RelativeOffset must be a multiple of 8 bits.
        template <size_t RelativeOffset>
        constexpr auto static_offset_bits() const
        {
            return static_offset<RelativeOffset / 8>();
        }

        template <int Offset>
        using static_offset_t
            = aligned_ptr<Type, residue::template offset_class<Offset>::divisor, residue::template offset_class<Offset>::remainder,
                          typename OffsetRange::template offset_range<Offset>>;

        // static_offset
        // Offset this pointer by a static amount of bytes.
        //
        template <int Offset>
        static_offset_t<Offset> static_offset() const
        {
            return static_offset_t<Offset>(get_offset<Offset>()
#ifdef NETSER_DEREFERENCE_LOGGING
                                               ,
                                           logger_
#endif
            );
        }

        // stride
        // Offset this pointer by a dynamic amount of static strides in bytes.
        template <size_t StrideBytes>
        auto stride(size_t index) const
        {
            return aligned_ptr<Type, gcd(get_access_alignment(0), power2_alignment_of(StrideBytes))>(
                reinterpret_cast<Type *>(reinterpret_cast<copy_constness_t<Type, char> *>(ptr_) + StrideBytes * index)
#ifdef NETSER_DEREFERENCE_LOGGING
                    ,
                logger_
#endif
            );
        }

        // stride_offset
        template <size_t StrideBytes, int RelativeOffsetBytes>
        auto stride_offset(size_t index)
        {
            return static_offset<RelativeOffsetBytes>().template stride<StrideBytes>(index);
        }

#ifdef NETSER_DEBUG_CONSOLE
        void describe()
        {
            std::cout << "Alignment: " << pointer_alignment;
            if (Defect != 0)
                std::cout << "(+" << Defect << ")";
        }
#endif

        type ptr_;

#ifdef NETSER_DEREFERENCE_LOGGING
        dereference_logger *logger_;
#endif
    };

    template <size_t Alignment, size_t Defect = 0, int Offset = 0, typename OffsetRange = lower_bounded<0>, typename Arg>
    auto make_aligned_ptr(Arg *arg
#ifdef NETSER_DEREFERENCE_LOGGING
                          ,
                          dereference_logger *logger = nullptr
#endif
    )
    {
#ifndef NETSER_DEREFERENCE_LOGGING
        return aligned_ptr<Arg, Alignment, Defect, OffsetRange>(arg).template static_offset<Offset>();
#else
        return aligned_ptr<Arg, Alignment, Defect, OffsetRange>(arg, logger).template static_offset<Offset>();
#endif
    }

    template <int Offset, size_t Alignment, size_t Defect, typename OffsetRange, typename Type>
    auto offset(aligned_ptr<Type, Alignment, Defect, OffsetRange> ptr)
    {
        return ptr.template static_offset<Offset>();
    }

    namespace detail
    {

        template <typename AlignedPtr, int OffsetBytes>
        struct range_filter_move
        {
            template <typename AccessType>
            struct filter
            {
                static constexpr bool value = (AlignedPtr::get_max_alignment() % AccessType::alignment != 0)
                                              || !AlignedPtr::offset_range::contains(
                                                  AlignedPtr::align_down(OffsetBytes, AccessType::alignment),
                                                  AlignedPtr::align_down(OffsetBytes + int(AccessType::size), AccessType::alignment));
            };
        };

        template <typename AlignedPtr, int OffsetBytes>
        struct range_filter_nomove
        {
            template <typename AccessType>
            struct filter
            {
                static constexpr bool value = (AlignedPtr::get_access_alignment(OffsetBytes) % AccessType::alignment != 0)
                                              || !AlignedPtr::offset_range::contains(OffsetBytes, OffsetBytes + int(AccessType::size));
            };
        };

    } // namespace detail

    // filtered_accesses_t
    // Filters out accesses from a list of accesses that would violate alignment and range restrictions
    // of the given aligned pointer.
    template <typename AlignedPtr, int OffsetBytes, typename AccessList>
    using filtered_accesses_t = meta::type_list::erase_if<platform_memory_accesses, detail::range_filter_move<AlignedPtr, OffsetBytes>::template filter>;

    // filtered_accesses_t
    // Filters out accesses that cannont be done
    //
    template <typename AlignedPtr, int OffsetBytes, typename AccessList>
    using filtered_accesses_nomove_t
        = meta::type_list::erase_if<platform_memory_accesses, detail::range_filter_nomove<AlignedPtr, OffsetBytes>::template filter>;
} // namespace netser

#endif
