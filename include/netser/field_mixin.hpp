#ifndef META_FIELD_MIXIN_HPP___
#define META_FIELD_MIXIN_HPP___

#include <netser/layout.hpp>

namespace netser {

    namespace detail
    {

        // =====================================
        // simple_field_layout_mixin
        // crtp helper for leaf layouts (fields)
        template <typename Field>
        struct simple_field_layout_mixin
        {
            template <typename AlignedPtrType>
            static AlignedPtrType transform_buffer(AlignedPtrType ptr)
            {
                return ptr;
            }
        };

    } // namespace detail

}

#endif