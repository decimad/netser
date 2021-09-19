#ifndef NETSER_LAYOUT_NODE_HPP___
#define NETSER_LAYOUT_NODE_HPP___

#include "meta/util.hpp"
#include <concepts>
#include <type_traits>
#include <meta/tlist.hpp>
#include <meta/iterator.hpp>
#include <meta/tree.hpp>
#include <netser/utility.hpp>

namespace netser {

    namespace concepts {

        template<typename T>
        concept LayoutNode = requires(T a) {
            true;
        };

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

//    template<meta::concepts::Iterator TreeIterator, size_t Offset>
//    auto advance(netser::layout_meta_iterator<TreeIterator, Offset>) ->
//        netser::layout_meta_iterator<meta::advance_t<TreeIterator>, (Offset + meta::dereference_t<TreeIterator>::size)>;
//
//    template<typename TreeIterator, size_t Offset>
//    auto dereference(netser::layout_meta_iterator<TreeIterator, Offset>) -> detail::placed_field<meta::dereference_t<TreeIterator>, Offset>;

    template <typename TreeIterator, size_t Offset>
    auto is_sentinel(netser::layout_meta_iterator<TreeIterator, Offset>) -> decltype(is_sentinel(std::declval<TreeIterator>()));

    template<typename Layout>
    using layout_meta_begin_t = layout_meta_iterator< meta::tree_begin<Layout, layout_tree_ctx, meta::traversals::lr> >;

    template<typename Layout>
    using layout_enumerator_t = layout_meta_iterator< meta::tree_begin<Layout, layout_tree_ctx, meta::traversals::lr> >;

//    template<typename Layout>
//    using layout_meta_end_t   = layout_meta_iterator< meta::tree_end<Layout, layout_tree_ctx, meta::traversals::lr> >;
//
//    template<typename Layout>
//    using layout_meta_range_t = meta::iterator_range<layout_meta_begin_t<Layout>, layout_meta_end_t<Layout>>;

//    template<meta::concepts::Iterator Iter, size_t Offset1, size_t Offset2>
//    std::true_type iterator_equal(layout_meta_iterator<Iter, Offset1>, layout_meta_iterator<Iter, Offset2>);

}

#endif
