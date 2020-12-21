#ifndef NETSER_RESERVED_HPP___
#define NETSER_RESERVED_HPP___

#include "netser/fwd.hpp"

namespace netser {

    template <size_t Bits>
    struct reserved
    {
        static constexpr size_t size  = Bits;
        static constexpr size_t count = 1;
    };

    static_assert(concepts::LayoutSpecifier<reserved<0>>);

}

#endif