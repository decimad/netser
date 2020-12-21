#ifndef NETSER_LAYOUT_NODE_HPP___
#define NETSER_LAYOUT_NODE_HPP___

#include <concepts>
#include <type_traits>
#include <meta/tlist.hpp>
#include <netser/layout_iterator.hpp>

namespace netser {

    namespace concepts {

        template<typename T>
        concept LayoutNode = requires(T a) {
            { T::size }  -> std::convertible_to<const size_t>;
            { T::count } -> std::convertible_to<const size_t>;
        };

    }

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

        template<meta::concepts::TypeList List, bool Empty = meta::type_list::is_empty<List>>
        struct count_fields_size_struct
        {
            static constexpr size_t count = 0;
            static constexpr size_t size = 0;
        };

        template<template<typename...> typename List, typename... Elems>
        struct count_fields_size_struct<List<Elems...>, false>
        {
            static constexpr size_t count = (Elems::count + ...);
            static constexpr size_t size  = (Elems::size  + ...);
        };

    } // namespace detail

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

}

#endif
