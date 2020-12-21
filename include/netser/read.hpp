//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_READ_HPP__
#define NETSER_READ_HPP__

#include <netser/platform.hpp>
#include <netser/layout_iterator.hpp>

namespace netser
{

    namespace detail
    {

        template <typename ZipIterator, bool IsEnd = ZipIterator::is_end>
        struct read_zip_iterator_struct
        {
            static NETSER_FORCE_INLINE auto read(ZipIterator it)
            {
                using type = decltype(deref_t<typename ZipIterator::layout_iterator::ct_iterator>::field::read_span(it));
                auto result = deref_t<typename ZipIterator::layout_iterator::ct_iterator>::field::read_span(it);
                return read_zip_iterator_struct<type>::read(result);
            }
        };

        template <typename ZipIterator>
        struct read_zip_iterator_struct<ZipIterator, true>
        {
            static NETSER_FORCE_INLINE auto read(ZipIterator it)
            {
                static_assert(ZipIterator::layout_iterator::ct_iterator::offset % 8 == 0, "Must!");
                // return a pointer one behind the last field (for continuation)
                return it.layout().get().template static_offset<int(ZipIterator::layout_iterator::ct_iterator::offset) / 8>();
            }
        };

        template <typename ZipIterator>
        NETSER_FORCE_INLINE auto read_zip_iterator(ZipIterator it)
        {
            // Need partial specialization because on each non-empty level, the iterators need to be dereferenced (which is not valid for
            // the iterator == end)
            return read_zip_iterator_struct<ZipIterator>::read(it);
        }

    } // namespace detail

    // read< Layout, Mapping >( source : aligned_ptr<>, dest : Dest& )
    //
    //
    template <typename Layout, typename Mapping, typename AlignedPtr, typename Arg>
    void read(AlignedPtr ptr, Arg &&dest)
    {
        detail::read_zip_iterator(
            make_zip_iterator(make_layout_iterator<Layout>(ptr), make_mapping_iterator<Mapping>(std::forward<Arg>(dest))));
    }

    // read< Layout, Mapping >( source : aligned_ptr<>, dest : Dest& )
    //
    //
    template <typename Layout, typename Mapping, typename AlignedPtr, typename Arg>
    NETSER_FORCE_INLINE auto read_inline(AlignedPtr ptr, Arg &&dest)
    {
        return detail::read_zip_iterator(
            make_zip_iterator(make_layout_iterator<Layout>(ptr), make_mapping_iterator<Mapping>(std::forward<Arg>(dest))));
    }

} // namespace netser

#endif
