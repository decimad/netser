//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_LAYOUT_HPP__
#define NETSER_LAYOUT_HPP__

#include <netser/utility.hpp>
#include <meta/tree.hpp>
#include <type_traits>

namespace netser
{
    using std::size_t;

    namespace concepts {

        template<typename T>
        concept LayoutNode = requires(T a) {
            true;
        };

        template<typename T>
        concept LayoutNodeArray = std::is_array<T>::value && concepts::LayoutNode<std::remove_all_extents_t<T>>;

        template<typename T>
        concept LayoutSpecifier = concepts::LayoutNode<T> || LayoutNodeArray<T>;

    }

    namespace detail {

        template <typename Field, size_t Offset>
        struct placed_field : public Field
        {
            using field = Field;
            static constexpr size_t offset = Offset;
            // static constexpr size_t size = field::size;
            static constexpr netser::bit_range range = netser::make_bit_range<offset, offset + field::size>();
        };

    }

    struct layout_tree_ctx
    {
        template<concepts::LayoutNode Node, size_t Pos>
        struct get_child {
            using type = typename Node::template get_child<Pos>;
        };

        template<concepts::LayoutNode Node>
        struct num_children {
            static constexpr size_t value = Node::num_children;
        };
    };

    // Layout tree iterator wrapper that keeps track of the bit position inside the layout
    template<typename TreeIterator, size_t Offset = 0>
    struct layout_meta_iterator {
        using iterator = TreeIterator;
        using dereference = detail::placed_field<meta::dereference_t<TreeIterator>, Offset>;
        using advance     = netser::layout_meta_iterator<meta::advance_t<TreeIterator>, (Offset + meta::dereference_t<TreeIterator>::size)>;

        static constexpr size_t get_offset()
        {
            return Offset;
        }
    };

    template<meta::concepts::Sentinel TreeIterator, size_t Offset>
    struct layout_meta_iterator<TreeIterator, Offset> {
        using iterator = TreeIterator;

        static constexpr size_t get_offset()
        {
            return Offset;
        }
    };

    template <typename TreeIterator, size_t Offset>
    auto is_sentinel(netser::layout_meta_iterator<TreeIterator, Offset>) -> decltype(is_sentinel(std::declval<TreeIterator>()));

    template<typename Layout>
    using layout_meta_begin_t = layout_meta_iterator< meta::tree_begin<Layout, layout_tree_ctx, meta::traversals::lr> >;

    template<typename Layout>
    using layout_enumerator_t = layout_meta_iterator< meta::tree_begin<Layout, layout_tree_ctx, meta::traversals::lr> >;

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








    template <typename T, size_t Size, size_t UnrollMax = 7>
    struct array_layout;

    namespace detail {

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

    }

    //
    // A layout is basically a tree structure with the data-fields as leafs.
    //
    template <concepts::LayoutSpecifier... Layouts>
    struct layout
    {
        using transformed_list = meta::type_list::transform<meta::tlist<Layouts...>, detail::layout_transformer>;

      public:
        template<size_t Pos>
        using get_child = meta::type_list::get<transformed_list, Pos>;

        static constexpr size_t num_children = meta::type_list::size<transformed_list>;

      public:
        template <size_t SourceAlignment, size_t Index, size_t Offset>
        auto read(const void *src);

        template <size_t DestAlignment, size_t Index, size_t Offset, typename Type>
        void write(void *dest, Type &&val);
    };

} // namespace netser

#endif
