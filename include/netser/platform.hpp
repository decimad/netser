#ifndef NETSER_PLATFORM_ACCESS_HPP___
#define NETSER_PLATFORM_ACCESS_HPP___

#include "netser_config.hpp"

namespace netser {

    // =================================
    // legal_memory_accesses
    // list of memory accesses possible given a certain buffer alignment
    template <size_t Alignment>
    using legal_memory_accesses = meta::type_list::copy_if<platform_memory_accesses, detail::alignment_matches<Alignment>::template type>;

    namespace detail
    {

        template <bool Predicate>
        struct byte_swap_switch_struct
        {
            template <typename T>
            static NETSER_FORCE_INLINE T swap(T val)
            {
                return byte_swap(byte_swap_wrapper<T>(val));
            }
        };

        template <>
        struct byte_swap_switch_struct<false>
        {
            template <typename T>
            static NETSER_FORCE_INLINE T swap(T val)
            {
                return val;
            }
        };

    } // namespace detail

    template <bool Predicate, typename T>
    T NETSER_FORCE_INLINE conditional_swap(T val)
    {
        return detail::byte_swap_switch_struct<Predicate>::swap(val);
    }

}

#endif