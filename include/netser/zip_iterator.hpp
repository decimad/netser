//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_ZIP_ITERATOR_HPP__
#define NETSER_ZIP_ITERATOR_HPP__

#include <utility>
#include <meta/iterator.hpp>

namespace netser {

    // Zipped Iterator
    template< typename LayoutIterator, typename MappingIterator >
    struct zip_iterator
    {
        constexpr zip_iterator( LayoutIterator lay_it, MappingIterator map_it )
            : layout_( lay_it ), mapping_( map_it )
        {}

        using layout_iterator  = LayoutIterator;
        using mapping_iterator = MappingIterator;

        static constexpr bool is_end = meta::concepts::Sentinel<LayoutIterator> || meta::concepts::Sentinel<MappingIterator>;

    private:
        const layout_iterator  layout_;
        const mapping_iterator mapping_;

        using next_layout_iterator  = decltype(layout_.advance());
        using next_mapping_iterator = decltype(mapping_.advance());
        using next_iterator_type    = zip_iterator< next_layout_iterator, next_mapping_iterator >;

    public:
        constexpr auto advance() const
        {
            static_assert(!is_end, "Advancing an end iterator!");

            if constexpr (!is_end)
            {
                return zip_iterator< next_layout_iterator, next_mapping_iterator >(
                    layout_.advance(),
                    mapping_.advance()
                );
            }
        }

        constexpr auto dereference() const {
            return std::make_pair( layout_.dereference(), mapping_.dereference() );
        }

        constexpr layout_iterator layout() const
        {
            return layout_;
        }

        constexpr mapping_iterator mapping() const
        {
            return mapping_;
        }
    };

    template< typename ZipIterator >
    using zip_field_t = typename meta::dereference_t<typename ZipIterator::layout_iterator>::field;

    template< typename ZipIterator >
    using zip_mapping_t = typename ZipIterator::mapping_iterator::dereference;

    template< typename LayoutIterator, typename MappingIterator >
    constexpr zip_iterator< LayoutIterator, MappingIterator > make_zip_iterator( LayoutIterator lay_it, MappingIterator map_it )
    {
        //static_assert(is_layout_iterator_ct<LayoutIterator::ct_iterator>::value, "Not a layout iterator!");
        //static_assert(is_mapping_iterator_ct<MappingIterator::ct_iterator>::value, "Not a mapping iterator!");
        return zip_iterator< LayoutIterator, MappingIterator >( lay_it, map_it );
    }

}

#endif
