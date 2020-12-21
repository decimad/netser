//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_LAYOUT_HPP__
#define NETSER_LAYOUT_HPP__

#include "fwd.hpp"
#include <netser/aligned_ptr.hpp>
#include <netser/mapping.hpp>
#include <meta/tlist.hpp>
#include <netser/utility.hpp>
#include <netser/zip_iterator.hpp>
#include <netser/layout_iterator.hpp>
#include <netser/layout_node.hpp>
#include <type_traits>

namespace netser
{
    template <typename T, size_t Size, size_t UnrollMax = 8>
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
        static constexpr size_t count = detail::count_fields_size_struct<transformed_list>::count;
        static constexpr size_t size = detail::count_fields_size_struct<transformed_list>::size;

        //
        // Get nth field
        //
      private:
        template <size_t Index, size_t IndexOffset, size_t BitOffset, typename LayoutList>
        struct get_field_struct
        {
        };

        template <size_t Index, size_t IndexOffset, size_t BitOffset, typename Layout0, typename... LayoutsTail>
        struct get_field_struct<Index, IndexOffset, BitOffset, meta::tlist<Layout0, LayoutsTail...>>
            : public std::conditional_t<
                  (Index < IndexOffset + Layout0::count), typename Layout0::template get_field<Index - IndexOffset, BitOffset>,
                  get_field_struct<Index, IndexOffset + Layout0::count, BitOffset + Layout0::size, meta::tlist<LayoutsTail...>>>
        {
        };

      private:
        template <typename LayoutList>
        struct pop_struct
        {
            using result = meta::error_type;
        };

        template <typename Type0, typename... Types>
        struct pop_struct<meta::tlist<Type0, Types...>>
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

} // namespace netser

#endif
