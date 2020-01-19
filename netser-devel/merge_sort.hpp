#pragma once
#include <netser/type_list.hpp>

//
// Not meant seriously.
//

namespace netser
{

    template <typename ListA, typename ListB, template <typename, typename> class Compare, typename ResultList = clear_t<ListA>,
              bool Stop = is_empty_v<ListA> || is_empty_v<ListB>>
    struct merge
    {
        using type = std::conditional_t<Compare<front_t<ListA>, front_t<ListB>>::value,
                                        typename merge<pop_front_t<ListA>, ListB, Compare, push_back_t<ResultList, front_t<ListA>>>::type,
                                        typename merge<ListA, pop_front_t<ListB>, Compare, push_back_t<ResultList, front_t<ListB>>>::type>;
    };

    template <typename ListA, typename ListB, template <typename, typename> class Compare, typename ResultList>
    struct merge<ListA, ListB, Compare, ResultList, true>
    {
        using type = concat_t<ResultList, ListA, ListB>;
    };

    template <typename ListA, typename ListB, template <typename, typename> class Compare>
    using merge_t = typename merge<ListA, ListB, Compare>::type;

    namespace detail4
    {

        template <typename A, typename B>
        struct pair
        {
            using first = A;
            using second = B;
        };

        template <typename List, typename ListA, typename ListB, bool TakeA = true>
        struct split;

        template <typename List, typename ListA, typename ListB>
        struct split<List, ListA, ListB, true>
        {
            using type = typename split<pop_front_t<List>, push_back_t<ListA, front_t<List>>, ListB, false>::type;
        };

        template <typename List, typename ListA, typename ListB>
        struct split<List, ListA, ListB, false>
        {
            using type = typename split<pop_front_t<List>, ListA, push_back_t<ListB, front_t<List>>, true>::type;
        };

        template <template <typename...> class ListType, typename ListA, typename ListB>
        struct split<ListType<>, ListA, ListB, true>
        {
            using type = pair<ListA, ListB>;
        };

        template <template <typename...> class ListType, typename ListA, typename ListB>
        struct split<ListType<>, ListA, ListB, false>
        {
            using type = pair<ListA, ListB>;
        };

    } // namespace detail4

    template <typename List>
    using split = typename detail4::split<List, clear_t<List>, clear_t<List>>::type;

    template <typename List, template <typename, typename> class Compare>
    struct merge_sort
    {
        using a = typename split<List>::first;
        using b = typename split<List>::second;

        using a_sorted = typename merge_sort<a, Compare>::type;
        using b_sorted = typename merge_sort<b, Compare>::type;

        using type = merge_t<a_sorted, b_sorted, Compare>;
    };

    template <template <typename...> class ListType, typename Type0, template <typename, typename> class Compare>
    struct merge_sort<ListType<Type0>, Compare>
    {
        using type = ListType<Type0>;
    };

    template <template <typename...> class ListType, template <typename, typename> class Compare>
    struct merge_sort<ListType<>, Compare>
    {
        using type = ListType<>;
    };

    template <typename List, template <typename, typename> class Compare>
    using merge_sort_t = typename merge_sort<List, Compare>::type;

} // namespace netser
