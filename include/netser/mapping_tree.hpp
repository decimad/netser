#ifndef NETSER_MAPPING_TREE_HPP___
#define NETSER_MAPPING_TREE_HPP___

#include <meta/tree.hpp>

namespace netser {

    /* we choose our mapping tree to be of intrusive type */

    template<typename Mapping>
    using mapping_begin_t = meta::tree_begin<Mapping, meta::contexts::intrusive, meta::traversals::lr>;

    template<typename Mapping>
    using mapping_end_t   = meta::tree_end<Mapping, meta::contexts::intrusive, meta::traversals::lr>;

    template<typename Mapping>
    using mapping_range_t = meta::tree_range_t<Mapping, meta::contexts::intrusive, meta::traversals::lr>;

    //template<typename MappingRange>
    //using mapping_path_range_t = meta::tree_range_t<MappingRange>;

    template<meta::concepts::Enumerator MappingPath, typename T>
    auto&& dereference(T&& arg)
    {
        if constexpr (!meta::concepts::Sentinel<MappingPath>)
        {
            return dereference<meta::advance_t<MappingPath>>(
                meta::dereference_t<MappingPath>::apply(std::forward<T>(arg))
            );
        }
        else
        {
            return arg;
        }
    }

}

#endif