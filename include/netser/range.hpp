//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_RANGE_HPP__
#define NETSER_RANGE_HPP__

#include <netser/type_list.hpp>

namespace netser {

    // compile-time
    namespace ct {

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
        template< typename Begin, typename End >
        struct iterator_range {
            using begin = Begin;
            using end   = End;

            using advance = iterator_range< typename Begin::advance, end >;
            using dereference = deref_t< Begin >;

            static constexpr bool empty() {
                return false;
            }
        };

        template< typename Iterator >
        struct iterator_range< Iterator, Iterator > {
            using begin = Iterator;
            using end   = Iterator;

            static constexpr bool empty() {
                return true;
            }
        };

        // concat_range
        // Note: since we cannot expect a range to have type-list style variadic arguments
        //       we have to work with the iterators and range concept only.
        //
        namespace detail {

            template< typename... Ranges >
            struct concat_range_iterator
            {
            };

            template< typename Range0, typename... Ranges >
            struct concat_range_iterator< Range0, Ranges... >
            {
                using dereference = deref_t< Range0 >;

                using advance = std::conditional_t< !next_t< Range0 >::empty(),
                    concat_range_iterator< next_t< Range0 >, Ranges... >,
                    concat_range_iterator< Ranges... >
                >;
            };

            template< typename T >
            struct is_empty_range {
                static constexpr bool value = T::empty();
            };

            template< typename... Ranges >
            struct concat_range
            {
                struct empty_type {};

                using begin = empty_type;
                using end   = empty_type;

                static constexpr bool empty()
                {
                    return true;
                }
            };

            template< typename Range0, typename... Ranges >
            struct concat_range< Range0, Ranges... >
            {
                using begin = concat_range_iterator< Range0, Ranges... >;
                using end   = concat_range_iterator< >;

                using dereference = deref_t< Range0 >;

                using advance = std::conditional_t< !next_t<Range0>::empty(),
                    concat_range< next_t< Range0 >, Ranges... >,
                    concat_range< Ranges... >
                >;

                static constexpr bool empty()
                {
                    return false;
                }
            };

            // Remove all empty ranges before "returning" the concat range.
            template< typename... Ranges >
            using make_concat_range = erase_if_t< concat_range< Ranges... > , is_empty_range >;
        }

        template< typename... Ranges >
        using concat_range = detail::make_concat_range< Ranges... >;


        //
        // Make a range from any type compatible with the TypeList concept
        // (no need to use for class type_list, it defines those already)
        //

        namespace detail {

            template< typename T >
            struct make_type_list_range;

            template< template< typename... > class TypeList, typename... Types >
            struct make_type_list_range< TypeList< Types... > >
            {
                using type = iterator_range< type_list_iterator< Types... >, type_list_iterator<> >;
            };

        }

        template< typename TypeList >
        using type_list_range_t = typename detail::make_type_list_range< TypeList >::type;

        // for_each
        //
        //
        template< template< typename > class Functor, typename Begin >
        void for_each( Begin, Begin )
        {}

        template< template< typename > class Functor, typename Begin, typename End >
        void for_each( Begin, End )
        {
            Functor<deref_t<Begin>>()();
            for_each< Functor >( next_t<Begin>(), End() );
        }

        template< template< typename > class Functor, typename Range >
        void for_each( Range )
        {
            for_each< Functor >( typename Range::begin(), typename Range::end() );
        }

        // Range iterator
        //
        // Fixme: Range::empty() doesn't work on MSVC, cannot see an error though.
        template< typename Range, bool IsEmpty = Range::empty() >
        struct range_iterator
        {
            using dereference = deref_t<Range>;
            using advance     = typename next_t<Range>::begin;
        };

        template< typename Range >
        struct range_iterator< Range, true >
        {
        };

        struct empty_range {
            static constexpr bool empty()
            {
                return true;
            }
        };

        struct nonempty_range {
            static constexpr bool empty()
            {
                return false;
            }
        };


        // Integral range
        template< typename T, T Begin, T End >
        struct integral_range : public nonempty_range {
            using dereference = std::integral_constant<T, Begin>;
            using advance = integral_range< T, Begin + 1, End >;

            using begin = integral_range;
            using end   = integral_range< T, End, End >;
        };

        template< typename T, T End >
        struct integral_range< T, End, End > : public empty_range
        {
            using begin = integral_range;
            using end   = integral_range;

        };

    }

}



#endif


