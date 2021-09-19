//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_READ_HPP__
#define NETSER_READ_HPP__

#include <netser/platform.hpp>
#include <netser/layout_iterator.hpp>

#include <meta/iterator.hpp>
#include <type_traits>

namespace netser
{

    namespace detail
    {

        template <typename ZipIterator, bool IsEnd = ZipIterator::is_end>
        struct read_zip_iterator_struct
        {
            static NETSER_FORCE_INLINE auto read(ZipIterator it)
            {
                using type = decltype(meta::dereference_t<typename ZipIterator::layout_iterator::range>::read_span(it));
                auto result = meta::dereference_t<typename ZipIterator::layout_iterator::range>::read_span(it);
                return read_zip_iterator_struct<type>::read(result);
            }
        };

        template <typename ZipIterator>
        struct read_zip_iterator_struct<ZipIterator, true>
        {
            static NETSER_FORCE_INLINE auto read(ZipIterator it)
            {
                static_assert(offset_v<typename ZipIterator::layout_iterator::range> % 8 == 0, "Must!");
                // return a pointer one behind the last field (for continuation)
                return it.layout().get().template static_offset<int(offset_v<typename ZipIterator::layout_iterator::range>) / 8>();
            }
        };

        template <typename ZipIterator>
        NETSER_FORCE_INLINE auto read_zip_iterator(ZipIterator it)
        {
            // Need partial specialization because on each non-empty level, the iterators need to be dereferenced (which is not valid for
            // the iterator == end)
            return read_zip_iterator_struct<ZipIterator>::read(it);
        }


        // Discussion:
        // - Is it worth it to define a zip-iterator?
        //   Yes - this way a single return type can define both the state of the layout as well as of the mapping range.
        // - Is it worth it to wrap the pointer into a runtime zip iterator?
        //   Yes, because this way the return value of a typed read operation can be fed into the generic read function, providing all
        //   necessary information in one object/type.

        template<meta::concepts::Enumerator LayoutRange, meta::concepts::Enumerator MappingRange, typename AlignedPtr, typename Dest>
        NETSER_FORCE_INLINE auto read_range_span(AlignedPtr src, Dest& destination)
        {
            return read_range_span_typed<LayoutRange, MappingRange>(src, destination);
        }

        template <meta::concepts::Enumerator LayoutRange, meta::concepts::Enumerator MappingRange, typename AlignedPtr, typename Dest>
        NETSER_FORCE_INLINE auto read_range(AlignedPtr src, Dest& destination)
        {
            return read_range( read_range_span(src, destination) );
        }

        template<meta::concepts::Sentinel LayoutRange, meta::concepts::Sentinel MappingRange, typename AlignedPtr, typename Dest>
        NETSER_FORCE_INLINE void read_range(AlignedPtr src [[maybe_unused]], Dest& destination [[maybe_unused]])
        {
        }

    } // namespace detail

    // read< Layout, Mapping >( source : aligned_ptr<>, dest : Dest& )
    //
    //
    template <typename Layout, typename Mapping, typename AlignedPtr, typename Arg>
    void read(AlignedPtr ptr, Arg &&dest)
    {
        detail::read_zip_iterator(
            make_zip_iterator(
                make_layout_iterator<Layout>(ptr),
                make_mapping_iterator<Mapping>(std::forward<Arg>(dest))
                )
            );
    }

    // read< Layout, Mapping >( source : aligned_ptr<>, dest : Dest& )
    //
    //
    template <typename Layout, typename Mapping, typename AlignedPtr, typename Arg>
    NETSER_FORCE_INLINE auto read_inline(AlignedPtr ptr, Arg &&dest)
    {
        return detail::read_zip_iterator(
            make_zip_iterator(
                make_layout_iterator<Layout>(ptr),
                make_mapping_iterator<Mapping>(std::forward<Arg>(dest))
                )
            );
    }

} // namespace netser

#endif
