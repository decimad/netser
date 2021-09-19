//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_LAYOUT_ITERATOR_HPP__
#define NETSER_LAYOUT_ITERATOR_HPP__

#include <meta/tlist.hpp>
#include <netser/layout_tree.hpp>
#include <netser/utility.hpp>
#include <type_traits>
#include <utility>

namespace netser
{
    namespace detail
    {
/*
        template <typename Field, size_t Offset>
        struct placed_field : public Field
        {
            using field = Field;
            static constexpr size_t offset = Offset;
            // static constexpr size_t size = field::size;
            static constexpr bit_range range = make_bit_range<offset, offset + field::size>();
        };
*/

        namespace impl
        {

            template <typename Field, size_t Offset>
            std::true_type is_placed_field_func(placed_field<Field, Offset>);

            template <typename T>
            std::false_type is_placed_field_func(T);

        } // namespace impl

        template <typename T>
        using is_placed_field = decltype(impl::is_placed_field_func(std::declval<T>()));

    } // namespace detail

    // layout_sequence_value_type
    // layout_iterator_ct dereferences into an instance of this template, so they are the value_type of the layout container.
    // member-fn transform_buffer can be used to adjust the buffer pointer in case of dynamic-length-fields
    //
    template <typename Field, size_t Offset>
    using layout_sequence_value_type = detail::placed_field<Field, Offset>;

    // Previous sizes:
    // .text = 42130

    //
    // layout_iterator
    // runtime equivalent of layout_iterator, keeping along an aligned buffer pointer.
    template <typename AlignedPtr, meta::concepts::Enumerator LayoutRange>
    struct layout_iterator
    {
        constexpr layout_iterator(AlignedPtr ptr) : buffer_(ptr)
        {
        }

        static constexpr bool is_end = false;

        using pointer_type = AlignedPtr;

        // compile time layout iterator range describing the remaining layout fields
        using range = LayoutRange;

        // advance
        // return the iterator to the next field in the layout sequence. possibly advance the buffer pointer while doing so.
        constexpr auto advance() const
        {
            if constexpr(!meta::concepts::EmptyRange<range>)
            {
                return layout_iterator<AlignedPtr, meta::advance_t<range>>(meta::dereference_t<range>::field::transform_buffer(buffer_));

/*
    LayoutRange = meta::iterator_range<                                                                                                                                                                                                                                                                  !!!v!!!
                    netser::layout_meta_iterator<meta::sentinel<meta::tree_iterator<netser::layout_tree_ctx, meta::tlist<netser::int_<false, 32, netser::byte_order::big_endian>, meta::detail::SE<netser::layout<netser::int_<false, 32, netser::byte_order::big_endian> >, 0> >, meta::traversals::lr> >, 32>,
                    netser::layout_meta_iterator<meta::sentinel<meta::tree_iterator<netser::layout_tree_ctx, meta::tlist<netser::int_<false, 32, netser::byte_order::big_endian>, meta::detail::SE<netser::layout<netser::int_<false, 32, netser::byte_order::big_endian> >, 0> >, meta::traversals::lr> >, 0> >
*/
            }
            else
            {
                return;
            }
        }

        static constexpr size_t get_pointer_alignment()
        {
            return AlignedPtr::get_pointer_alignment();
        }

        static constexpr size_t get_max_alignment()
        {
            return AlignedPtr::get_max_alignment();
        }

        static constexpr size_t get_offset()
        {
            return offset_v<range>;
        }

        static constexpr size_t get_access_alignment(size_t additional_offset_bits)
        {
            return AlignedPtr::get_access_alignment((int) ((get_offset() + additional_offset_bits) / 8u));
        }

        // get
        // return the stored aligned buffer pointer
        constexpr AlignedPtr get() const
        {
            return buffer_;
        }

        const AlignedPtr buffer_;
    };

    //
    // layout_iterator
    // runtime equivalent of layout_iterator, keeping along an aligned buffer pointer.
    template <typename AlignedPtr, meta::concepts::Sentinel LayoutRange>
    struct layout_iterator<AlignedPtr, LayoutRange>
    {
        using pointer_type = AlignedPtr;

        layout_iterator(AlignedPtr ptr) : buffer_(ptr)
        {
        }

        static constexpr bool is_end = true;

        // compile time layout iterator range describing the remaining layout fields
        using range = LayoutRange;

        // advance
        // return the iterator to the next field in the layout sequence. possibly advance the buffer pointer while doing so.
        auto advance() const
        {
            return meta::error_type();
        }

        // alignment
        // memory alignment of the stored aligned buffer pointer
        static constexpr size_t get_pointer_alignment()
        {
            return AlignedPtr::get_pointer_alignment();
        }

        static constexpr size_t get_pointer_max_alignment()
        {
            return AlignedPtr::get_pointer_max_alignment();
        }

        static constexpr size_t get_pointer_defect()
        {
            return AlignedPtr::get_pointer_defect();
        }

        static constexpr size_t get_pointer_static_offset()
        {
            return AlignedPtr::get_static_offset();
        }

        static constexpr size_t get_access_alignment(size_t offset_bytes)
        {
            return AlignedPtr::get_access_alignment(offset_bytes);
        }

        static constexpr size_t get_offset()
        {
            return 0;
        }

        // get
        // return the stored aligned buffer pointer
        AlignedPtr get() const
        {
            return buffer_;
        }

        const AlignedPtr buffer_;
    };

    // make_layout_iterator
    // create layout iterator from layout and aligned buffer pointer
    //
    template <typename Layout, size_t Offset = 0, typename AlignedPtr>
    constexpr auto make_layout_iterator(AlignedPtr ptr)
        -> layout_iterator<typename AlignedPtr::template static_offset_t<Offset>, layout_enumerator_t<Layout>>
    {
        return layout_iterator<typename AlignedPtr::template static_offset_t<Offset>, layout_enumerator_t<Layout>>(ptr);
    }

    template <typename AlignedPtr, meta::concepts::Enumerator LayoutRange>
    auto dereference(netser::layout_iterator<AlignedPtr, LayoutRange>) -> meta::dereference_t<LayoutRange>;

    template<typename AlignedPtr, meta::concepts::Enumerator LayoutRange>
    auto advance(netser::layout_iterator<AlignedPtr, LayoutRange> iter)
    {
        return iter.advance();
    }

    template <typename AlignedPtr, meta::concepts::Enumerator LayoutRange>
    auto is_sentinel(netser::layout_iterator<AlignedPtr, LayoutRange>) -> std::integral_constant<bool, meta::concepts::Sentinel<LayoutRange>>;
}


#endif
