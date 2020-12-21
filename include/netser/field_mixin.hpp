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
            using pop = detail::layout_pop_result<Field, layout<>>;

            template <size_t Offset>
            struct next_offset
            {
                static constexpr size_t value = Offset + Field::size;
            };

            template <typename AlignedPtrType>
            static AlignedPtrType transform_buffer(AlignedPtrType ptr)
            {
                return ptr;
            }
        };

    } // namespace detail

}

#endif