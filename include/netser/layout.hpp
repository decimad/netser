//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_LAYOUT_HPP__
#define NETSER_LAYOUT_HPP__

#include "netser_config.hpp"
#include <netser/aligned_ptr.hpp>
#include <netser/mapping.hpp>
#include <netser/type_list.hpp>
#include <netser/utility.hpp>
#include <netser/zip_iterator.hpp>
#include <type_traits>


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
            using range = bit_range<offset, offset + field::size>;
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
        template <typename Field, size_t Offset, typename SegmentRange>
        struct placed_field_segment : public placed_field<Field, Offset>
        {
            static_assert(SegmentRange::begin <= Field::size && SegmentRange::end <= Field::size, "Bad field segment!");
            using segment_range = SegmentRange;
        };

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

    template <typename T, size_t Size, size_t UnrollMax = 8>
    struct array_layout;

    namespace detail
    {

        // template< typename Field, typename TailLayout = empty_layout > struct layout_pop_result
        // Every layout must return an instance of this upon a pop operation
        // TailLayout == empty_layout means that the layout is empty after the pop operation
        template <typename Field, typename TailLayout>
        struct layout_pop_result
        {
            using field = Field;
            using tail_layout = TailLayout;
            static constexpr bool tail_empty = TailLayout::count == 0;
        };

        namespace impl
        {

            template <typename Field, typename TailLayout>
            std::true_type is_layout_pop_result_struct(layout_pop_result<Field, TailLayout>);

            template <typename T>
            std::false_type is_layout_pop_result_struct(T);

        } // namespace impl

        template <typename T>
        using is_layout_pop_result = decltype(impl::is_layout_pop_result_struct(std::declval<T>()));

        template <typename T>
        struct layout_transformer
        {
            using type = T;
        };

        template <typename T, size_t Size>
        struct layout_transformer<T[Size]>
        {
            using type = array_layout<T, Size>;
        };

    } // namespace detail

    //
    // A layout is basically a tree structure with the data-fields as leafs.
    //
    template <typename... Layouts>
    struct layout
    {
        using transformed_list = transform_t<type_list<Layouts...>, detail::layout_transformer>;

        //
        //  Determine Size and Count of contained fields and/or layouts.
        //
      private:
        template <typename List, typename Bogus = void>
        struct count_fields_size_struct
#ifdef _MSC_VER
        { // prevent warning C4348 on MS Visual C++ (Bogus is needed on gcc, because there must not be full specializations of member class
          // templates).
            static_assert(std::is_same<Bogus, void>::value, "Bad usage!");
        }
#endif
        ;

        template <typename Bogus>
        struct count_fields_size_struct<type_list<>, Bogus>
        {
            static constexpr size_t count = 0;
            static constexpr size_t size = 0;
        };

        template <typename Layout0, typename... LayoutsTail, typename Bogus>
        struct count_fields_size_struct<type_list<Layout0, LayoutsTail...>, Bogus>
        {
            using tail = count_fields_size_struct<type_list<LayoutsTail...>>;
            static constexpr size_t count = Layout0::count + tail::count;
            static constexpr size_t size = Layout0::size + tail::size;
        };

      public:
        static constexpr size_t count = count_fields_size_struct<transformed_list>::count;
        static constexpr size_t size = count_fields_size_struct<transformed_list>::size;

        //
        // Get nth field
        //
      private:
        template <size_t Index, size_t IndexOffset, size_t BitOffset, typename LayoutList>
        struct get_field_struct
        {
        };

        template <size_t Index, size_t IndexOffset, size_t BitOffset, typename Layout0, typename... LayoutsTail>
        struct get_field_struct<Index, IndexOffset, BitOffset, type_list<Layout0, LayoutsTail...>>
            : public std::conditional_t<
                  (Index < IndexOffset + Layout0::count), typename Layout0::template get_field<Index - IndexOffset, BitOffset>,
                  get_field_struct<Index, IndexOffset + Layout0::count, BitOffset + Layout0::size, type_list<LayoutsTail...>>>
        {
        };

      private:
        template <typename LayoutList>
        struct pop_struct
        {
            using result = error_type;
        };

        template <typename Type0, typename... Types>
        struct pop_struct<type_list<Type0, Types...>>
        {
            using child_pop_result = typename Type0::pop;
            using result = std::conditional_t<
                child_pop_result::tail_empty,
                // Type0 is empty after this pop
                detail::layout_pop_result<typename child_pop_result::field, layout<Types...>>,
                // Type0 has still elements remaining after this pop
                detail::layout_pop_result<typename child_pop_result::field, layout<typename child_pop_result::tail_layout, Types...>>>;
        };

        // Pop field
      public:
        using pop = typename pop_struct<transformed_list>::result;

        // Iterator sequence
      public:
        using begin = layout_iterator_ct<layout>;
        using end = layout_iterator_ct<layout<>, size, 0>;

      public:
        template <size_t Index, size_t BitOffset = 0>
        using get_field = get_field_struct<Index, 0, BitOffset, transformed_list>;

        template <size_t SourceAlignment, size_t Index, size_t Offset>
        auto read(const void *src);

        template <size_t DestAlignment, size_t Index, size_t Offset, typename Type>
        void write(void *dest, Type &&val);
    };

    namespace detail
    {

        template <typename T>
        struct is_inner_layout_node_checker
        {
            using begin = typename T::begin;
            using end = typename T::end;
            using pop = typename T::pop;

            static_assert(is_layout_iterator_ct<begin>::value, "");
            static_assert(is_layout_iterator_ct<end>::value, "");
            static_assert(is_layout_pop_result<pop>::value, "");

            static constexpr size_t count = T::count;
            static constexpr size_t size = T::size;
        };

        template <typename T, typename S = is_inner_layout_node_checker<T>>
        std::true_type is_inner_layout_node_func(T, size_t foo = S::size);

        template <typename T>
        std::false_type is_inner_layout_node_func(T, T = T());

        template <typename T>
        using is_inner_layout_node = decltype(is_inner_layout_node_func(std::declval<T>()));

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
        using advance = error_type;
        using dereference = error_type;

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
            return AlignedPtr::get_access_alignment((get_offset() + additional_offset_bits) / 8);
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
        using dereference = error_type;

        static constexpr bool is_end = ct_iterator::is_end;

        // advance
        // return the iterator to the next field in the layout sequence. possibly advance the buffer pointer while doing so.
        auto advance() const
        {
            return error_type();
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

    namespace detail
    {

        // =====================================
        // simple_field_layout_mixin
        // crtp helper for leaf layouts (fields)
        template <typename Field>
        struct simple_field_layout_mixin
        {
            using pop = detail::layout_pop_result<Field, layout<>>;

            template <size_t Offset>
            struct next_offset
            {
                static constexpr size_t value = Offset + Field::size;
            };

            template <typename AlignedPtrType>
            static AlignedPtrType transform_buffer(AlignedPtrType ptr)
            {
                return ptr;
            }
        };

    } // namespace detail

    template <size_t Bits>
    struct reserved
    {
    };

} // namespace netser

#endif
