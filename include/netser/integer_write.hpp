//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_INTEGER_WRITE_HPP__
#define NETSER_INTEGER_WRITE_HPP__

#include <netser/integer_shared.hpp>
#include <netser/range.hpp>

namespace netser
{

    //=============
    // write_span helper types
    //

    namespace detail
    {

        // discover_span_size
        // get the size of the (compatible) integer span given an iterator to the first integer
        template <typename Endianess>
        struct is_endianess_integer_field
        {
            template <typename T>
            struct condition_inner
            {
                static constexpr bool value = false;
            };

            template <bool Signed, size_t Size, typename InnerEndianess>
            struct condition_inner<int_<Signed, Size, InnerEndianess>>
            {
                static constexpr bool value = std::is_same<Endianess, InnerEndianess>::value;
            };

            template <typename T>
            struct condition
            {
                static constexpr bool value = condition_inner<typename T::field>::value;
            };
        };

        template <typename Integer, typename State>
        struct discover_write_span_size_body
        {
            using type = std::integral_constant<size_t, Integer::size + State::value>;
        };

        template <typename CtRange>
        using discover_write_span_size =
            typename while_<CtRange,
                            is_endianess_integer_field<typename deref_t<CtRange>::endianess>::template condition, // take the endianess of
                                                                                                                  // the first element.
                            discover_write_span_size_body, std::integral_constant<size_t, 0>>::type;

        enum class discover_case
        {
            add_field,
            grow_write,
            finish
        };

        template <typename List, bool Empty = is_empty<List>::value>
        struct front_size_or_zero
        {
            static constexpr size_t value = 0;
        };

        template <typename List>
        struct front_size_or_zero<List, false>
        {
            static constexpr size_t value = front_t<List>::size;
        };

        template <typename CtLayoutIterator, typename AccessList, size_t SpanSize, size_t FieldWrittenBits, size_t CollectedBits = 0>
        struct discover_access;

        template <typename CtLayoutIterator, typename AccessList, size_t SpanSize, size_t FieldWrittenBits, size_t CollectedBits,
                  discover_case Case>
        struct discover_switch;

        template <typename CtLayoutIterator, typename AccessList, size_t SpanSize, size_t FieldWrittenBits, size_t CollectedBits>
        struct discover_switch<CtLayoutIterator, AccessList, SpanSize, FieldWrittenBits, CollectedBits, discover_case::add_field>
        {
            using type = typename discover_access<next_t<CtLayoutIterator>, AccessList, SpanSize, 0,
                                                  CollectedBits + deref_t<CtLayoutIterator>::size - FieldWrittenBits>::type;
        };

        template <typename CtLayoutIterator, typename AccessList, size_t SpanSize, size_t FieldWrittenBits, size_t CollectedBits>
        struct discover_switch<CtLayoutIterator, AccessList, SpanSize, FieldWrittenBits, CollectedBits, discover_case::grow_write>
        {
            using type =
                typename discover_access<CtLayoutIterator, pop_front_t<AccessList>, SpanSize, FieldWrittenBits, CollectedBits>::type;
        };

        template <typename CtLayoutIterator, typename AccessList, size_t SpanSize, size_t FieldWrittenBits, size_t CollectedBits>
        struct discover_switch<CtLayoutIterator, AccessList, SpanSize, FieldWrittenBits, CollectedBits, discover_case::finish>
        {
            using type = front_t<AccessList>;
        };

        // FieldWrittenBits = bits of the Layout front that have already been written in a preceding access
        // AccessList       = list of access, sorted in ascending size (This list must have been cleared of accesses that are unaligned wrt.
        // the layout-iterator) WriteBits        = The amount of bits that have been assembled for this write so far
        template <typename CtLayoutIterator, typename AccessList, size_t SpanSize, size_t FieldWrittenBits, size_t CollectedBits>
        struct discover_access
        {
            static_assert(!is_empty<AccessList>::value, "No access possible.");
            static_assert(!CtLayoutIterator::is_end, "Could not get enough fields");
            static_assert(!is_integer_v<CtLayoutIterator>, "No integer field");

            using write = front_t<AccessList>;
            using field = deref_t<CtLayoutIterator>; // => placed_field

            static constexpr size_t field_remaining_bits = field::size - FieldWrittenBits;

            static constexpr bool is_finished = field_remaining_bits + CollectedBits == write::size * 8;
            static constexpr bool need_more_data = field_remaining_bits + CollectedBits < write::size * 8;
            static constexpr bool need_more_write = field_remaining_bits + CollectedBits > write::size * 8;
            static constexpr bool can_grow_write
                = (list_size<AccessList>::value) > 1 && front_size_or_zero<pop_front_t<AccessList>>::value * 8 <= SpanSize;

            static constexpr discover_case this_case = (is_finished || (need_more_write && !can_grow_write))
                                                           ? discover_case::finish
                                                           : (need_more_data ? discover_case::add_field : discover_case::grow_write);

            using type = typename discover_switch<CtLayoutIterator, AccessList, SpanSize, FieldWrittenBits, CollectedBits, this_case>::type;
        };

        struct write_integer_algorithm
        {
          private:
            enum class execute_action
            {
                write,
                write_and_continue,
                collect_all,
                collect_partial,
                error
            };

            static constexpr execute_action determine_execute_action(size_t access_size, size_t access_written, size_t field_size,
                                                                     size_t field_written)
            {
                return (access_written > access_size)
                           ? execute_action::error
                           : ((access_written == access_size)
                                  ? ((field_written == 0) ? execute_action::write : execute_action::write_and_continue)
                                  : ((access_written + field_size - field_written <= access_size) ? execute_action::collect_all
                                                                                                  : execute_action::collect_partial));
            }

            template <typename PlacedAccess, typename Endianess, size_t FieldSize, size_t BitsWritten = 0, size_t FieldWritten = 0,
                      execute_action Task = determine_execute_action(PlacedAccess::size * 8, BitsWritten, FieldSize, FieldWritten)>
            struct execute_access
            {

                template <typename T>
                static void execute()
                {
                }
            };

            // Access complete, no partial field remaining
            template <typename PlacedAccess, typename Endianess, size_t FieldSize, size_t BitsWritten, size_t FieldWritten>
            struct execute_access<PlacedAccess, Endianess, FieldSize, BitsWritten, FieldWritten, execute_action::write>
            {
                using type = typename PlacedAccess::type;

                template <typename ZipIterator>
                NETSER_FORCE_INLINE static auto execute(ZipIterator it, type val = 0)
                {
                    using access_endianess = typename PlacedAccess::endianess;

                    static_assert(FieldWritten == 0, "Huh?");

                    PlacedAccess::template write(it.layout().get(),
                                                 conditional_swap<!std::is_same<access_endianess, Endianess>::value>(val));

                    return it;
                }

                template <typename ZipIterator>
                static auto describe(ZipIterator it, type val = 0)
                {
                    //                    static_assert(!ZipIterator::is_end, "Invalid iterator!");
                    PlacedAccess::describe();
                    return it;
                }
            };

            // Access complete, partial field remaining
            template <typename PlacedAccess, typename Endianess, size_t FieldSize, size_t BitsWritten, size_t FieldWritten>
            struct execute_access<PlacedAccess, Endianess, FieldSize, BitsWritten, FieldWritten, execute_action::write_and_continue>
            {
                using type = typename PlacedAccess::type;
                using access_endianess = typename PlacedAccess::endianess;

                template <typename ZipIterator>
                NETSER_FORCE_INLINE static auto execute(ZipIterator it, type val = 0)
                {
                    PlacedAccess::template write(it.layout().get(),
                                                 conditional_swap<!std::is_same<access_endianess, Endianess>::value>(val));

#ifdef NETSER_DEBUG_CONSOLE
                    PlacedAccess::describe();
#endif
                    return write_integer<FieldWritten>(it);
                }
            };

            // Get next data (all or all remaining) -> iterator advances on this step and current data doesn't need to be shifted down.
            // Get next data (partial) -> iterator _does not_ advance on this step and current data needs to be shifted down
            // Layout field a, Memory access x
            // layout: ...aaaaaaaabbbbbbbb...        -> done
            // layout: ...aaaabbbbbbbbbbbb...        -> continue and cap on next step
            // access: ...xxxxxxxx]                  (shift down 5, mask 8 bits)
            template <typename PlacedAccess, typename Endianess, size_t FieldSize, size_t BitsWritten, size_t FieldWritten>
            struct execute_access<PlacedAccess, Endianess, FieldSize, BitsWritten, FieldWritten, execute_action::collect_all>
            {
                using type = typename PlacedAccess::type;
                static constexpr size_t num_bits = FieldSize - FieldWritten;
                static constexpr size_t shift_up = PlacedAccess::size * 8 - BitsWritten - num_bits;

                template <typename ZipIterator>
                NETSER_FORCE_INLINE static auto execute(ZipIterator it, type val = 0)
                {
                    return execute_access<PlacedAccess, Endianess, FieldSize, BitsWritten + num_bits, 0>::template execute(
                        it.advance(), val | ((bit_mask<type>(num_bits) & static_cast<type>(it.mapping().dereference())) << shift_up));
                }
            };

            // Get next data (partial) -> iterator _does not_ advance on this step and current data needs to be shifted down
            // Layout field a, Memory access x
            // layout: ...aaaaaaaaaaaaa...
            // access: ...xxxxxxxx]                  (shift down 5, mask 8 bits)
            template <typename PlacedAccess, typename Endianess, size_t FieldSize, size_t BitsWritten, size_t FieldWritten>
            struct execute_access<PlacedAccess, Endianess, FieldSize, BitsWritten, FieldWritten, execute_action::collect_partial>
            {
                template <size_t bits_to_write, size_t field_remaining>
                struct validate : public std::true_type
                {
                    static_assert(bits_to_write < field_remaining, "Huh?");
                };

                using type = typename PlacedAccess::type;
                template <typename ZipIterator>
                NETSER_FORCE_INLINE static auto execute(ZipIterator it, type val = 0)
                {
                    static constexpr size_t num_bits = PlacedAccess::size * 8 - BitsWritten;
                    static_assert(validate<num_bits, FieldSize - FieldWritten>::value, "Something's wrong!");
                    static constexpr size_t shift_down = FieldSize - FieldWritten - num_bits;

#ifdef NETSER_DEBUG_CONSOLE
                    std::cout << "Shifting down by " << shift_down << " bits.";
#endif

                    return execute_access<PlacedAccess, Endianess, FieldSize, BitsWritten + num_bits,
                                          FieldWritten + num_bits>::template execute(it,
                                                                                     val
                                                                                         | (bit_mask<type>(num_bits)
                                                                                            & static_cast<type>(it.mapping().dereference()
                                                                                                                >> shift_down)));
                }
            };

          public:
            template <size_t FieldWritten = 0, typename ZipIterator>
            NETSER_FORCE_INLINE static auto write_integer(ZipIterator it)
            {
                using layout_iterator = typename ZipIterator::layout_iterator;
                using layout_iterator_ct = typename layout_iterator::ct_iterator;
                using placed_field = deref_t<layout_iterator_ct>;
                using field = typename placed_field::field;
                using ptr_type = typename layout_iterator::pointer_type;
                constexpr size_t span_size = detail::discover_write_span_size<layout_iterator_ct>::value - FieldWritten;

#ifdef NETSER_DEBUG_CONSOLE
                std::cout << "\n    Aligned Ptr: ";
                it.layout().get().describe();
                std::cout << "\n    Field offset bytes: " << layout_iterator_ct::offset / 8 << ". ";
                std::cout << "\n    Access alignment: " << layout_iterator::get_access_alignment(FieldWritten)
                          << " (Field written: " << FieldWritten << "), ";
#endif

                using access = typename discover_access<
                    layout_iterator_ct,
                    filtered_accesses_nomove_t<ptr_type, (layout_iterator::get_offset() + FieldWritten) / 8, platform_memory_accesses>,
                    span_size, FieldWritten>::type;

#ifdef NETSER_DEBUG_CONSOLE
                std::cout << "\n    Would like to use size " << access::size << "\n";
#endif

                using placed_access = placed_atomic_memory_access<access, placed_field::offset + FieldWritten>;

                static_assert((placed_field::offset + FieldWritten) % 8 == 0, "Something's bad!");

                return execute_access<placed_access, typename field::endianess, field::size, 0, FieldWritten>::template execute(it);
            }
        };

    } // namespace detail

    template <bool Signed, size_t Bits, typename ByteOrder>
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

} // namespace netser

#endif
