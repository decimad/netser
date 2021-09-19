//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_TREE_ITERATOR_HPP__
#define NETSER_TREE_ITERATOR_HPP__

#include <meta/tlist.hpp>
#include <netser/utility.hpp>

namespace netser
{

    template<typename Node, size_t Pos>
    struct StackElement2
    {
        using node = Node;
        static constexpr size_t pos = Pos;

        using dereference = typename node::template child<pos>;
    };

    namespace detail {

        struct begin_type {
            static constexpr size_t children = 0;
            using dereference = begin_type;
        };

        struct end_type {
            static constexpr size_t children = 0;
            using dereference = end_type;
        };

        template<meta::concepts::TypeList Stack, bool Empty = meta::type_list::is_empty<Stack>>
        struct walk_stack_right_helper
        {
            using front = meta::type_list::front<Stack>;    // StackElement!

            using type = std::conditional_t<(front::node::children > (front::pos + 1)),
                // Replace top
                meta::type_list::push_front<meta::type_list::pop_front<Stack>, StackElement2<typename front::node, front::pos+1>>,
                // Pop top, continue
                typename walk_stack_right_helper<meta::type_list::pop_front<Stack>>::type
            >;
        };

        template<meta::concepts::TypeList Stack>
        struct walk_stack_right_helper<Stack, true> {
            using type = Stack;
        };

        template<meta::concepts::TypeList Stack, meta::concepts::TypeList NewStack = typename walk_stack_right_helper<Stack>::type, bool NewStackEmpty = meta::type_list::is_empty<NewStack>>
        struct walk_stack_right {
            using stack = NewStack;
            using node  = typename meta::type_list::front<NewStack>::dereference;
        };

        template<meta::concepts::TypeList Stack, meta::concepts::TypeList NewStack>
        struct walk_stack_right<Stack, NewStack, true>
        {
            using stack = tlist<>;
            using node  = end_type;
        };
    }

    template<typename Tree, typename Node, meta::concepts::TypeList PathStack = meta::tlist<>, bool Leaf = Node::children == 0>
    struct tree_iterator
    {
        using tree = Tree;
        using dereference = Node;

        using next = tree_iterator<Tree,
            typename Node::template child<0>,
            meta::type_list::push_front<PathStack, StackElement2<Node, 0>>
            >;
    };

    template<typename Tree, typename Node, meta::concepts::TypeList PathStack>
    struct tree_iterator<Tree, Node, PathStack, true>
    {
        // walk up the pathstack until we find an element that has a successor
        using next = tree_iterator<Tree, typename detail::walk_stack_right<PathStack>::node, typename detail::walk_stack_right<PathStack>::stack >;
    };

    template<typename Tree>
    struct tree_iterator<Tree, detail::begin_type, meta::tlist<>, true>
    {
        using dereference = detail::begin_type;
    };

    template<typename Tree>
    struct tree_iterator<Tree, detail::end_type, meta::tlist<>, true>
    {
        using dereference = detail::end_type;
    };


    template<typename Tree>
    using begin_iterator = tree_iterator<Tree, Tree>;

    template<typename Tree>
    using end_iterator = tree_iterator<Tree, detail::end_type, meta::tlist<>, true>;

    template<typename Tree, typename Node, typename Stack, bool IsLeaf>
    struct dereference<tree_iterator<Tree, Node, Stack, IsLeaf>>
    {
        using type = Node;
    };

    template<typename Tree, typename Node, typename Stack, bool IsLeaf>
    struct advance<tree_iterator<Tree, Node, Stack, IsLeaf>>
    {
        using type = tree_iterator<Tree, Node, Stack, IsLeaf>::next;
    };

    //
    //  tree(it)
    //
    template<typename TreeIterator>
    struct tree;

    template<typename Tree, typename Node, typename Stack>
    struct tree<tree_iterator<Tree, Node, Stack>>
    {
        using type = Tree;
    };

    template<typename TreeIterator>
    using tree_t = typename tree<TreeIterator>::type;

    //
    // node(it)
    //
    template<typename TreeIterator>
    struct node;

    template<typename Tree, typename Node, typename Stack>
    struct node<tree_iterator<Tree, Node, Stack>>
    {
        using type = Node;
    };

    template<typename TreeIterator>
    using node_t = typename node<TreeIterator>::type;




}

#endif
