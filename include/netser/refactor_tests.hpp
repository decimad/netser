#ifndef NETSER_REFACTOR_TESTS_HPP___
#define NETSER_REFACTOR_TESTS_HPP___

#include <netser/integer.hpp>
#include <netser/netser.hpp>

#include <meta/tlist.hpp>
#include <meta/util.hpp>
#include <meta/tree.hpp>
#include <meta/algorithm.hpp>

namespace netser {

    template<typename Iterator>
    struct is_leaf {
        // Warum funktioniert das, wenn Iterator eine Range ist?
        static constexpr bool value = (meta::node_num_children_v<meta::tree_context_t<Iterator>, meta::dereference_t<Iterator>> == 0);
    };

    template<typename Range>
    using leaf_list_t = meta::accumulate_t<meta::filter_range_t<Range, is_leaf>>;

    // -> hard limit = ~1015 fields
    //using my_layout = layout<layout<net_uint32, net_uint32>, layout<net_uint16, net_uint16>>;

    template<typename... Ts>
    struct test_tree
    {
        // can't use us on tree_iterator inside here, because the requires-clause becomes non-constants according to clang
    };

    using tree = test_tree<test_tree<net_uint32, net_uint16>, net_uint8>;

    static_assert(
        meta::are_same_v<
            leaf_list_t<meta::tree_range_t<tree, meta::tlist_tree_ctx>>,
            leaf_list_t<meta::tree_range_t<tree, meta::tlist_tree_ctx>>,
            meta::tlist<
                net_uint32, net_uint16, net_uint8
            >
        >
    );
}

#endif
