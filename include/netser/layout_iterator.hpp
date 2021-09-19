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
    struct layout_iterator : public LayoutRange
    {
        constexpr layout_iterator(AlignedPtr ptr) : buffer_(ptr)
        {
        }

        static constexpr bool is_end = false;

        using pointer_type = AlignedPtr;
        using iterator     = LayoutRange;

        // act like a layout_meta_iterator
        using typename LayoutRange::advance;
        using typename LayoutRange::dereference;
        using LayoutRange::get_offset;

        constexpr auto operator++() const
        {
            if constexpr(!meta::concepts::EmptyRange<LayoutRange>)
            {
                return layout_iterator<AlignedPtr, meta::advance_t<LayoutRange>>(meta::dereference_t<LayoutRange>::field::transform_buffer(buffer_));
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
    struct layout_iterator<AlignedPtr, LayoutRange> : public LayoutRange
    {
        using pointer_type = AlignedPtr;
        using iterator     = LayoutRange;

        // act like a layout_meta_iterator
        using LayoutRange::get_offset;

        layout_iterator(AlignedPtr ptr) : buffer_(ptr)
        {
        }

        static constexpr bool is_end = true;

        auto operator++() const
        {
            return meta::error_type();
        }

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
    auto is_sentinel(netser::layout_iterator<AlignedPtr, LayoutRange>) -> std::integral_constant<bool, meta::concepts::Sentinel<LayoutRange>>;
}


#endif
