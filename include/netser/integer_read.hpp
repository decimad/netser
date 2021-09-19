//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_INTEGER_READ_HPP__
#define NETSER_INTEGER_READ_HPP__

#include <netser/aligned_ptr.hpp>
#include <netser/integer_shared.hpp>
#include <netser/mem_access.hpp>
#include <netser/platform.hpp>

// Devdebt: Clean this up so that it can actually be put into integer.hpp

namespace netser
{

    //===========================
    // read_span helper types
    //
    namespace detail
    {

        //
        // partial_field_access
        //
        template <typename PlacedAccess, typename PlacedField>
        struct partial_field_access
        {
            using placed_access = PlacedAccess;
            using placed_field = PlacedField;

            static constexpr bit_range field_range = placed_field::range;
            using access_integral_type = typename placed_access::type;

            static constexpr bit_range access_range       = placed_access::range;
            static constexpr bit_range intersection_range = intersection<placed_access::range, field_range>();

            static constexpr byte_order access_endianess = placed_access::endianess;
            static constexpr byte_order field_endianess  = placed_field::field::endianess;

            static constexpr size_t post_read_shift_down
                = (field_range.end() < access_range.end()) ? (access_range.end() - field_range.end()) : 0;
            static constexpr size_t pre_assemble_shift_up
                = (access_range.end() < field_range.end()) ? (field_range.end() - access_range.end()) : 0;

            template <typename StageType, typename AlignedPtr>
            static StageType read(AlignedPtr ptr)
            {
                return static_cast<StageType>(
                           (conditional_swap<access_endianess != field_endianess>(placed_access::read(ptr))
                            >> post_read_shift_down)
                           & bit_mask<access_integral_type>(intersection_range.size()))
                       << pre_assemble_shift_up;
            }
        };

        //
        // better_access
        //

        namespace impl
        {
            template <typename PartialAccessA, typename PartialAccessB>
            using better_access_t = std::conditional_t<(PartialAccessA::intersection_range.size() > PartialAccessB::intersection_range.size()),
                                                        PartialAccessA, PartialAccessB>;

            //
            // find_best_access
            //
            template <typename LayoutIterator, int Offset, typename PossibleAccesses, typename CurrentBest = empty_access>
            struct find_best_access;

            template <typename LayoutIterator, int Offset, typename CurrentBest>
            struct find_best_access<LayoutIterator, Offset, meta::tlist<>, CurrentBest>
            {
                static_assert(!std::is_same<CurrentBest, empty_access>::value, "No matching read!");
                using type = CurrentBest;
                //static constexpr size_t next_offset = type::access_range::end;

#ifdef NETSER_DEBUG_CONSOLE
                static void describe()
                {
                    std::cout << "    ....Done. Best access size: " << type::placed_access::access::size << "("
                              << CurrentBest::access_range::begin << "..." << CurrentBest::access_range::end << ")\n";
                }
#endif
            };

            template <typename LayoutIterator, int Offset, typename Access0, typename... AccessList, typename CurrentBest>
            struct find_best_access<LayoutIterator, Offset, meta::tlist<Access0, AccessList...>, CurrentBest>
            {
                using call_type = find_best_access<
                    LayoutIterator, Offset, meta::tlist<AccessList...>,
                    better_access_t<partial_field_access<placed_memory_access<Access0, Offset, typename LayoutIterator::pointer_type>,
                                                         meta::dereference_t<LayoutIterator>>,
                                    CurrentBest>>;

                using type = typename call_type::type;

#ifdef NETSER_DEBUG_CONSOLE
                static void describe()
                {
                    std::cout << "    At offset " << Offset << " checking access with size " << Access0::size << ".\n";
                    call_type::describe();
                }
#endif
            };

        } // namespace impl

        template <typename LayoutIterator, int Offset,
                  typename PossibleAccessesList
                  = filtered_accesses_t<typename LayoutIterator::pointer_type, Offset / 8, platform_memory_accesses>,
                  typename CurrentResult = empty_access>
        using find_best_access_t = typename impl::find_best_access<LayoutIterator, Offset, PossibleAccessesList, CurrentResult>::type;

#ifdef NETSER_DEBUG_CONSOLE
        template <typename LayoutIterator, int Offset,
                  typename PossibleAccessesList
                  = filtered_accesses_t<typename LayoutIterator::pointer_type, Offset / 8, platform_memory_accesses>,
                  typename CurrentResult = empty_access>
        void describe_find_best_access_t()
        {
            std::cout << "\n    Finding best access at offset " << Offset << " out of "
                      << list_size_v<PossibleAccessesList> << " alternatives.\n";
            impl::find_best_access<LayoutIterator, Offset, PossibleAccessesList, CurrentResult>::describe();
        }
#endif

        namespace impl
        {
            template <typename LayoutIterator, typename StageType, int Offset, typename PartialFieldAccessList = meta::tlist<>,
                      bool IsFinished = (Offset >= meta::dereference_t<LayoutIterator>::range.end())>
            struct generate_memory_access_list_struct
            {
                using type = PartialFieldAccessList;

#ifdef NETSER_DEBUG_CONSOLE
                static void describe()
                {
                    std::cout << "Complete!\n";
                }
#endif
            };

            template <typename LayoutIterator, typename StageType, int Offset, typename... PartialFieldAccess>
            struct generate_memory_access_list_struct<LayoutIterator, StageType, Offset, meta::tlist<PartialFieldAccess...>, false>
            {
                static constexpr size_t alignment = LayoutIterator::get_access_alignment(Offset);
                using placed_field = meta::dereference_t<LayoutIterator>;

                using access = find_best_access_t<LayoutIterator, Offset>;
                using list = meta::tlist<PartialFieldAccess..., access>;
                using call_type = generate_memory_access_list_struct<LayoutIterator, StageType, access::access_range.end(), list>;
                using type = typename call_type::type;

#ifdef NETSER_DEBUG_CONSOLE
                static void describe()
                {
                    describe_find_best_access_t<LayoutIterator, Offset>();
                    std::cout << "    Access (" << access::access_range.begin() << "..." << access::access_range.end() << ") \n";
                    call_type::describe();
                }
#endif
            };

        } // namespace impl

        template <typename LayoutIterator, typename StageType>
        using generate_partial_memory_access_list_t =
            typename impl::generate_memory_access_list_struct<LayoutIterator, StageType, LayoutIterator::get_offset()>::type;

#ifdef NETSER_DEBUG_CONSOLE
        template <typename LayoutIterator, typename StageType>
        void describe_generate_partial_memory_access_list_t()
        {
            impl::generate_memory_access_list_struct<LayoutIterator, StageType, LayoutIterator::get_offset()>::describe();
        }
#endif

        // ===============
        // run_access_list
        // runs and assembles all partial_field_access'es in a list
        template <typename PartialFieldAccessList>
        struct run_access_list
        {
            template <typename StageType, typename AlignedPtr>
            static NETSER_FORCE_INLINE StageType run(AlignedPtr)
            {
                return StageType(0);
            }
#ifdef NETSER_DEBUG_CONSOLE
            template <typename StageType, typename AlignedPtr>
            static NETSER_FORCE_INLINE StageType run_describe(AlignedPtr)
            {
                return StageType(0);
            }
#endif
        };

        template <typename Access0, typename... Tail>
        struct run_access_list<meta::tlist<Access0, Tail...>>
        {
            template <typename StageType, typename AlignedPtr>
            static NETSER_FORCE_INLINE StageType run(AlignedPtr ptr)
            {
#ifdef NETSER_DEBUG_CONSOLE
                std::cout << "Access of size " << Access0::placed_access::access::size << "\n";
#endif
                return Access0::template read<StageType>(ptr) | run_access_list<meta::tlist<Tail...>>::template run<StageType>(ptr);
            }
        };

    } // namespace detail

    template <bool Signed, size_t Bits, byte_order ByteOrder>
    template <typename LayoutIterator>
    constexpr typename int_<Signed, Bits, ByteOrder>::integral_type
    int_<Signed, Bits, ByteOrder>::read(LayoutIterator layit)
    {
#ifdef NETSER_DEBUG_CONSOLE
        std::cout << "Generating memory access List (";
        std::cout << "Field size: " << meta::dereference_t<LayoutIterator>::size
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
        static_assert(!std::is_const<decltype(it.mapping().dereference())>::value, "Error!");

#ifdef NETSER_DEBUG_CONSOLE
        std::cout << "reading integer span:\n";
#endif
        it.mapping().dereference() = read(it.layout());

#ifdef NETSER_DEBUG_CONSOLE
        std::cout << "read span complete.\n";
#endif
        return it.advance();
    }

} // namespace netser

#endif
