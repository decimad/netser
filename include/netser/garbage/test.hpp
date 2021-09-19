#include <meta/tlist.hpp>
#include <type_traits>
#include <tuple>

namespace test {

    struct intrusive_tree_ctx {
        template<typename T>
        struct num_children {
            static constexpr size_t value = T::num_children;
        };

        template<typename T, size_t Index>
        struct get_child
        {
            using type = typename T::template child<Index>;
        };
    };

    struct end_type {
        static constexpr size_t num_children = 0;
        using dereference = end_type;
    };

    template<typename Tree, typename Node, typename Ctx = intrusive_tree_ctx>
    struct tree_iterator
    {
        using dereference = Node;
    };

    template<typename T, size_t NumChildren>
    struct next_tree_iter;

    template<typename Tree, typename Node, typename Ctx>
    struct next_tree_iter<tree_iterator<Tree, Node, Ctx>, 0>
    {
        using type = tree_iterator<Tree, end_type, Ctx>;
    };

    template<typename Tree, typename Node, typename Ctx, size_t NumChildren>
    struct next_tree_iter<tree_iterator<Tree, Node, Ctx>, NumChildren>
    {
        using type = tree_iterator<Tree, typename Ctx::template get_child<Node, 0>::type, Ctx>;
    };

    template<typename T>
    struct next_iter;

    template<typename Tree, typename Node, typename Ctx>
    struct next_iter<tree_iterator<Tree, Node, Ctx>>
    {
        using type = typename next_tree_iter<tree_iterator<Tree, Node, Ctx>, Ctx::template num_children<Node>::value>::type;
    };

//    template<typename Tree, typename Node, typename Ctx>
//    struct next_iter<tree_iterator<Tree, Node, Ctx, false>>
//    {
//        using type = tree_iterator<Tree, end_type, Ctx>;
//    };

    template<typename... Ts>
    struct test_tree
    {
        static constexpr size_t num_children = sizeof...(Ts);

        template<size_t Index>
        using child = std::remove_reference_t<decltype(std::get<Index>(std::declval<std::tuple<Ts...>>()))>;

        using begin = tree_iterator<test_tree, test_tree>;
        using end   = tree_iterator<test_tree, end_type>;
    };

    struct net_uint32 {
        static constexpr size_t num_children = 0;
        static constexpr size_t size = 32;
    };

    using my_layout = test_tree<test_tree<net_uint32>>;

    static_assert(
        std::is_same_v<
            typename next_iter<my_layout::begin>::type::dereference,
            test_tree<net_uint32>
        >
    );


    //using iter  = my_layout::begin;
    //using iter2 = iter::next;

}