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
#include <netser/layout_tree.hpp>
#include <meta/tree.hpp>
#include <type_traits>

namespace netser
{
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
