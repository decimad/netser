//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_RANDOM_INIT_HPP__
#define NETSER_RANDOM_INIT_HPP__

#include <meta/range.hpp>
#include <netser/utility.hpp>
#include <netser/zip_iterator.hpp>
#include <random>

// Utility algo to fill a given layout+mapping pair with random values inside the value range of the layout fields.
// This is useful during debugging because roundtrip tests for equality can only work if the layout is able to store the values of the
// mapping.
//
namespace netser
{

    namespace detail
    {

        template <typename LayoutIterator, typename MappingIterator, bool IsEnd = meta::concepts::EmptyRange<LayoutIterator> || MappingIterator::is_end>
        struct fill_random
        {
            template <typename Generator>
            static void _(MappingIterator it, Generator &&generator)
            {
                meta::dereference_t<LayoutIterator>::field::template fill_random(it, generator);
                using next_type = decltype(++it);
                return fill_random<meta::advance_t<LayoutIterator>, next_type>::template _(++it, std::forward<Generator>(generator));
            }
        };

        template <typename LayoutIterator, typename MappingIterator>
        struct fill_random<LayoutIterator, MappingIterator, true>
        {
            template <typename Generator>
            static void _(MappingIterator it, Generator &&generator)
            {
            }
        };

    } // namespace detail

    template <typename LayoutIterator, typename MappingIterator, typename Generator>
    void fill_mapping_random(MappingIterator mapping, Generator &&generator)
    {
        detail::fill_random<LayoutIterator, MappingIterator>::template _(mapping, std::forward<Generator>(generator));
    }

    template <typename LayoutIterator, typename MappingIterator>
    void fill_mapping_random(MappingIterator mapping)
    {
        std::random_device rd;
        std::mt19937 generator(rd());
        detail::fill_random<LayoutIterator, MappingIterator>::template _(mapping, generator);
    }

} // namespace netser

#endif
