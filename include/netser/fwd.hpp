//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_FWD_HPP___
#define NETSER_FWD_HPP___

#include <cstddef>
#include <type_traits>
#include "layout_node.hpp"

namespace netser
{
    using std::size_t;

    namespace concepts {

      template<typename T>
      concept LayoutNodeArray = std::is_array<T>::value && concepts::LayoutNode<std::remove_all_extents_t<T>>;

      template<typename T>
      concept LayoutSpecifier = concepts::LayoutNode<T> || LayoutNodeArray<T>;

    }

} // namespace netser

#endif
