//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_RANGE_HPP__
#define NETSER_RANGE_HPP__

#include "meta/common.hpp"
#include "netser/utility.hpp"
#include <meta/tlist.hpp>
#include <meta/range.hpp>


namespace netser
{
    /*

    // compile-time
    namespace ct
    {

        // Range concept:
        // must contain types
        //    begin - Iterator Concept
        //    end   - Iterator Concept
        // with next_t<... begin> == end
        //
        //    advance - Range Concept
        //    constexpr bool empty() return true iff the range is empty
        //

        // struct some_range {
        //      using begin = SomeIterator;
        //      using end   = SomeIterator;
        //      using advance = SomeRange;
        //      static constexpr bool empty() { return true/false; }
        //      using dereference = SomeType; // first type in range iff. empty() == false
        // };

        // iterator_range
        //
        // Begin and End must satisfy the Iterator Concept and belong to the same sequence
        template <typename Begin, typename End>
        struct iterator_range
        {
            using begin = Begin;
            using end = End;

            using advance = iterator_range<typename Begin::advance, end>;
            using dereference = deref_t<Begin>;

            static constexpr bool empty()
            {
                return false;
            }
        };

        template <typename Iterator>
        struct iterator_range<Iterator, Iterator>
        {
            using begin = Iterator;
            using end = Iterator;

            static constexpr bool empty()
            {
                return true;
            }
        };

        // concat_range
        // Note: since we cannot expect a range to have type-list style variadic arguments
        //       we have to work with the iterators and range concept only.
        //
        namespace detail
        {

            template <typename... Ranges>
            struct concat_range_iterator
            {
            };

            template <typename Range0, typename... Ranges>
            struct concat_range_iterator<Range0, Ranges...>
            {
                using dereference = deref_t<Range0>;

                using advance = std::conditional_t<!next_t<Range0>::empty(), concat_range_iterator<next_t<Range0>, Ranges...>,
                                                   concat_range_iterator<Ranges...>>;
            };

            template <typename T>
            struct is_empty_range
            {
                static constexpr bool value = T::empty();
            };

            template <typename... Ranges>
            struct concat_range
            {
                struct empty_type
                {
                };

                using begin = empty_type;
                using end = empty_type;

                static constexpr bool empty()
                {
                    return true;
                }
            };

            template <typename Range0, typename... Ranges>
            struct concat_range<Range0, Ranges...>
            {
                using begin = concat_range_iterator<Range0, Ranges...>;
                using end = concat_range_iterator<>;

                using dereference = deref_t<Range0>;

                using advance
                    = std::conditional_t<!next_t<Range0>::empty(), concat_range<next_t<Range0>, Ranges...>, concat_range<Ranges...>>;

                static constexpr bool empty()
                {
                    return false;
                }
            };

            // Remove all empty ranges before "returning" the concat range.
            template <typename... Ranges>
            using make_concat_range = meta::type_list::erase_if<concat_range<Ranges...>, is_empty_range>;
        } // namespace detail

        template <typename... Ranges>
        using concat_range = detail::make_concat_range<Ranges...>;


        template <meta::concepts::TypeList List>
        using type_list_range_t = iterator_range<meta::common::iterator<List, 0>, meta::common::iterator<List, meta::common::size_v<List>>>;;

        // for_each
        //
        //
        template <template <typename> class Functor, typename Begin>
        void for_each(Begin, Begin)
        {
        }

        template <template <typename> class Functor, typename Begin, typename End>
        void for_each(Begin, End)
        {
            Functor<deref_t<Begin>>()();
            for_each<Functor>(next_t<Begin>(), End());
        }

        template <template <typename> class Functor, typename Range>
        void for_each(Range)
        {
            for_each<Functor>(typename Range::begin(), typename Range::end());
        }

        // Range iterator
        //
        // Fixme: Range::empty() doesn't work on MSVC, cannot see an error though.
        template <typename Range, bool IsEmpty = Range::empty()>
        struct range_iterator
        {
            using dereference = deref_t<Range>;
            using advance = typename next_t<Range>::begin;
        };

        template <typename Range>
        struct range_iterator<Range, true>
        {
        };

        struct empty_range
        {
            static constexpr bool empty()
            {
                return true;
            }
        };

        struct nonempty_range
        {
            static constexpr bool empty()
            {
                return false;
            }
        };

        // Integral range
        template <typename T, T Begin, T End>
        struct integral_range : public nonempty_range
        {
            using dereference = std::integral_constant<T, Begin>;
            using advance = integral_range<T, Begin + 1, End>;

            using begin = integral_range;
            using end = integral_range<T, End, End>;
        };

        template <typename T, T End>
        struct integral_range<T, End, End> : public empty_range
        {
            using begin = integral_range;
            using end = integral_range;
        };

    } // namespace ct

    */

    // while_
    //
    //
    template <meta::concepts::Enumerator Range, template <typename> class Condition, template <typename, typename> class Body, typename State>
    struct while_;

    namespace detail
    {

/*
[build] ../netser-devel/../../meta/meta/iterator.hpp:38:17:   required for the satisfaction of 'ValidIterator<Iter>'
[with Iter =
    meta::iterator_range<
        netser::layout_meta_iterator<
            meta::tree_iterator<
                netser::layout_tree_ctx,
                meta::tlist<
                    netser::int_<false, 32, netser::byte_order::big_endian>,
                    meta::detail::SE<
                        netser::layout<
                            netser::int_<false, 4, netser::byte_order::big_endian>,
                            netser::int_<false, 4, netser::byte_order::big_endian>,
                            netser::int_<false, 4, netser::byte_order::big_endian>,
                            netser::int_<false, 4, netser::byte_order::big_endian>,
                            netser::int_<false, 16, netser::byte_order::big_endian>,
                            netser::int_<false, 8, netser::byte_order::big_endian>,
                            netser::int_<false, 8, netser::byte_order::big_endian>,
                            netser::int_<false, 8, netser::byte_order::big_endian>,
                            netser::int_<false, 8, netser::byte_order::big_endian>,
                            netser::int_<false, 64, netser::byte_order::big_endian>,
                            netser::int_<false, 32, netser::byte_order::big_endian>,
                            netser::layout<
                                netser::layout<
                                    netser::int_<false, 8, netser::byte_order::big_endian>[8]
                                >,
                                netser::int_<false, 16, netser::byte_order::big_endian>
                            >,
                            netser::int_<false, 16, netser::byte_order::big_endian>,
                            netser::int_<false, 8, netser::byte_order::big_endian>,
                            netser::int_<true, 8, netser::byte_order::big_endian>
                        >,
                        10
                    >
                >,
                meta::traversals::lr
            >,
            128
        >,
        netser::layout_meta_iterator<
            meta::sentinel<
                meta::tree_iterator<
                    netser::layout_tree_ctx,
                    meta::tlist<
                        netser::int_<true, 8, netser::byte_order::big_endian>,
                        meta::detail::SE<
                            netser::layout<
                                netser::int_<false, 4, netser::byte_order::big_endian>,
                                netser::int_<false, 4, netser::byte_order::big_endian>,
                                netser::int_<false, 4, netser::byte_order::big_endian>,
                                netser::int_<false, 4, netser::byte_order::big_endian>,
                                netser::int_<false, 16, netser::byte_order::big_endian>,
                                netser::int_<false, 8, netser::byte_order::big_endian>,
                                netser::int_<false, 8, netser::byte_order::big_endian>,
                                netser::int_<false, 8, netser::byte_order::big_endian>,
                                netser::int_<false, 8, netser::byte_order::big_endian>,
                                netser::int_<false, 64, netser::byte_order::big_endian>,
                                netser::int_<false, 32, netser::byte_order::big_endian>,
                                netser::layout<
                                    netser::layout<
                                        netser::int_<false, 8, netser::byte_order::big_endian>[8]
                                    >,
                                    netser::int_<false, 16, netser::byte_order::big_endian>
                                >,
                                netser::int_<false, 16, netser::byte_order::big_endian>,
                                netser::int_<false, 8, netser::byte_order::big_endian>,
                                netser::int_<true, 8, netser::byte_order::big_endian>
                            >,
                            14
                        >
                    >,
                    meta::traversals::lr
                >
            >,
            0
        >
    >]
*/


        template <typename Range, template <typename> class Condition, template <typename, typename> class Body, typename State,
                  bool Finish = !Condition<meta::dereference_t<Range>>::value>
        struct while_inner
        {
            using type = typename while_<meta::advance_t<Range>, Condition, Body, typename Body<meta::dereference_t<Range>, State>::type>::type;
        };

        template <typename Range, template <typename> class Condition, template <typename, typename> class Body, typename State>
        struct while_inner<Range, Condition, Body, State, true>
        {
            using type = State;
        };

    } // namespace detail

    template <meta::concepts::Enumerator Range, template <typename> class Condition, template <typename, typename> class Body, typename State>
    struct while_
    {
        using type = typename detail::while_inner<Range, Condition, Body, State>::type;
    };

    template <meta::concepts::Sentinel Range, template <typename> class Condition, template <typename, typename> class Body, typename State>
    struct while_<Range, Condition, Body, State>
    {
        using type = State;
    };
} // namespace netser

#endif
