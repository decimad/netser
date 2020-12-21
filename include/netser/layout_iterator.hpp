//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_LAYOUT_ITERATOR_HPP__
#define NETSER_LAYOUT_ITERATOR_HPP__

#include <netser/utility.hpp>

namespace netser
{

    //
    // layout_iterator_ct
    // Depth-first layout leaf (compile-time) iterator.
    //    ::dereference -> layout_sequence_value_type
    //    ::advance     -> next leaf as in depth first left before right order.
    template <typename Layout, size_t Offset = 0, size_t FieldCount = Layout::count>
    struct layout_iterator_ct;

    namespace detail
    {

        template <typename Field, size_t Offset>
        struct placed_field : public Field
        {
            using field = Field;
            static constexpr size_t offset = Offset;
            // static constexpr size_t size = field::size;
            static constexpr bit_range range = make_bit_range<offset, offset + field::size>();
        };

        namespace impl
        {

            template <typename Field, size_t Offset>
            std::true_type is_placed_field_func(placed_field<Field, Offset>);

            template <typename T>
            std::false_type is_placed_field_func(T);

        } // namespace impl

        template <typename T>
        using is_placed_field = decltype(impl::is_placed_field_func(std::declval<T>()));

        // TODO: -> detail
        /*
        template <typename Field, size_t Offset, typename SegmentRange>
        struct placed_field_segment : public placed_field<Field, Offset>
        {
            static_assert(SegmentRange::begin <= Field::size && SegmentRange::end <= Field::size, "Bad field segment!");
            using segment_range = SegmentRange;
        };
        */
    } // namespace detail

    namespace detail
    {

        namespace impl
        {

            template <typename Layout, size_t Offset, size_t FieldCount>
            std::true_type is_layout_iterator_ct_struct(layout_iterator_ct<Layout, Offset, FieldCount>);

            template <typename T>
            std::false_type is_layout_iterator_ct_struct(T);

        } // namespace impl

        template <typename T>
        using is_layout_iterator_ct = decltype(impl::is_layout_iterator_ct_struct(std::declval<T>()));

    } // namespace detail

    // layout_sequence_value_type
    // layout_iterator_ct dereferences into an instance of this template, so they are the value_type of the layout container.
    // member-fn transform_buffer can be used to adjust the buffer pointer in case of dynamic-length-fields
    //
    template <typename Field, size_t Offset>
    using layout_sequence_value_type = detail::placed_field<Field, Offset>;

    //
    // layout_iterator_ct
    // documentation see forward declaration
    //
    template <typename Layout, size_t Offset, size_t FieldCount /* = Layout::count */>
    struct layout_iterator_ct
    {
      private:
        using pop_result = typename Layout::pop;
        using field = typename pop_result::field;

      public:
        using dereference = layout_sequence_value_type<field, Offset>;
        using advance = layout_iterator_ct<typename pop_result::tail_layout, field::template next_offset<Offset>::value>;

        static constexpr size_t offset = Offset;

        static constexpr bool is_end = false;
        static constexpr bool empty()
        {
            return false;
        }
    };

    template <typename Layout, size_t Offset>
    struct layout_iterator_ct<Layout, Offset, 0>
    {
        using advance = meta::error_type;
        using dereference = meta::error_type;

        static constexpr size_t offset = Offset;
        static constexpr bool is_end = true;
        static constexpr bool empty()
        {
            return true;
        }
    };

    //
    // layout_iterator
    // runtime equivalent of layout_iterator, keeping along an aligned buffer pointer.
    template <typename AlignedPtr, typename CtIterator, bool IsEnd = CtIterator::is_end>
    struct layout_iterator
    {
        constexpr layout_iterator(AlignedPtr ptr) : buffer_(ptr)
        {
        }

        using pointer_type = AlignedPtr;

        // ct_iterator
        // compile time layout iterator desribing the current layout field
        using ct_iterator = CtIterator;
        using dereference = typename ct_iterator::dereference;

        // advance
        // return the iterator to the next field in the layout sequence. possibly advance the buffer pointer while doing so.
        constexpr auto advance() const
        {
            return layout_iterator<AlignedPtr, next_t<ct_iterator>>(deref_t<ct_iterator>::field::transform_buffer(buffer_));
        }

        static constexpr bool is_end = ct_iterator::is_end;

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
            return ct_iterator::offset;
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
    template <typename AlignedPtr, typename CtIterator>
    struct layout_iterator<AlignedPtr, CtIterator, true>
    {
        using pointer_type = AlignedPtr;

        layout_iterator(AlignedPtr ptr) : buffer_(ptr)
        {
        }

        // ct_iterator
        // compile time layout iterator desribing the current layout field
        using ct_iterator = CtIterator;
        using dereference = meta::error_type;

        static constexpr bool is_end = ct_iterator::is_end;

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
        -> layout_iterator<typename AlignedPtr::template static_offset_t<Offset>, layout_iterator_ct<Layout, Offset>>
    // cdt index parser cannont look at return statement in function body.
    {
        return layout_iterator<typename AlignedPtr::template static_offset_t<Offset>, layout_iterator_ct<Layout>>(ptr);
    }

}

#endif
