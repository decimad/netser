//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_ZIPPED_HPP__
#define NETSER_ZIPPED_HPP__

#include <type_traits>
#include <meta/tlist.hpp>
#include <netser/integer.hpp>
#include <netser/reserved.hpp>
#include <netser/layout_node.hpp>
#include <netser/layout_iterator.hpp>
#include <netser/mapping.hpp>
#include <netser/layout.hpp>

namespace netser
{

    template <typename... Args>
    struct zipped;

    template<typename T>
    static constexpr bool is_zipped_v = false;

    template<typename... T>
    static constexpr bool is_zipped_v<zipped<T...>> = true;

    namespace concepts {

        template<typename T>
        concept Zipped = is_zipped_v<T>;

        template<typename T, typename... Args>
        concept AutoZipped = requires(T a) {
            { default_zipped(std::declval<T>()) } -> Zipped;
        };

    }

    template<concepts::AutoZipped T>
    using auto_zipped_t = decltype(default_zipped(std::declval<T>()));

    template <auto Ptr, concepts::Zipped Zipped>
    requires (concepts::MemberPtr<Ptr>)
    struct zipped_member
    {
        using layout = typename Zipped::layout;
        using mapping = mapped_member<Ptr, typename Zipped::mapping>;
    };

    template<auto Ptr>
    requires (concepts::MemberPtr<Ptr> && concepts::AutoZipped<typename member_object_pointer_traits<Ptr>::value_type>)
    using auto_zipped_member = zipped_member<Ptr, auto_zipped_t<typename member_object_pointer_traits<Ptr>::value_type>>;

    namespace detail
    {
        namespace tlist = meta::type_list;

        template <typename Layout, typename Mapping, typename... Args>
        struct unzip
        {
            using layout = Layout;
            using mapping = Mapping;
        };

        template <typename Layout, typename Mapping, typename LayoutElement, typename MappingElement, typename... Tail>
        struct unzip_pair : public unzip<
            tlist::push_back<Layout,  LayoutElement>,
            tlist::push_back<Mapping, MappingElement>,
            Tail...>
        {
        };

        // Partial specialization for a <layout, struct-member-mapping> pair
        // If we do not match any of the specific partial specialization below, then treat any argument as a Layout after which a Mapping
        // must follow
        template <typename Layout, typename Mapping, typename LayoutArg, typename... Tail>
        struct unzip<Layout, Mapping, LayoutArg, Tail...> : public unzip_pair<Layout, Mapping, LayoutArg, Tail...>
        {
        };

        // Nested zipped-member specialization (cannot use concat, because hierarchy of nested members must be retained)
        template <typename Layout, typename Mapping, typename Zipped, auto Pointer, typename... Tail>
        struct unzip<Layout, Mapping, zipped_member<Pointer, Zipped>, Tail...>
            : public unzip<
                tlist::push_back<Layout,  typename zipped_member<Pointer, Zipped>::layout>,
                tlist::push_back<Mapping, typename zipped_member<Pointer, Zipped>::mapping>,
                Tail...>
        {
        };

        // "reserved" partial specialization (-> expands to a <net_uint, constant=0> pair).
        template <typename Layout, typename Mapping, size_t Bits, typename... Tail>
        struct unzip<Layout, Mapping, reserved<Bits>, Tail...>
            : public unzip<
                tlist::push_back<Layout,  net_uint<Bits>>,
                tlist::push_back<Mapping, constant<size_t, 0>>,
                Tail...>
        {
        };

        // Nested zipped partial specialization
        template <typename Layout, typename Mapping, typename... ZippedArgs, typename... Tail>
        struct unzip<Layout, Mapping, zipped<ZippedArgs...>, Tail...>
            : public unzip<
                tlist::push_back<Layout, typename zipped<ZippedArgs...>::layout>,
                tlist::push_back<Mapping, typename zipped<ZippedArgs...>::mapping>,
                Tail...>
        {
        };

    } // namespace detail

    template <typename... Args>
    struct zipped : public detail::unzip<layout<>, mapping_list<>, Args...>
    {
    };

    template <typename Zipped, typename AlignedPtr, typename Arg>
    void write_zipped(AlignedPtr dest, Arg &&src)
    {
        using layout = typename Zipped::layout;
        using mapping = typename Zipped::mapping;

        return write_inline<layout, mapping>(dest, std::forward<Arg>(src));
    }

    template <typename Zipped, typename AlignedPtr, typename Arg>
    auto read_zipped(AlignedPtr src, Arg &&dest)
    {
        using layout = typename Zipped::layout;
        using mapping = typename Zipped::mapping;

        return read_inline<layout, mapping>(src, std::forward<Arg>(dest));
    }

    template <typename Zipped, typename AlignedPtr, typename Arg>
    auto NETSER_FORCE_INLINE write_zipped_inline(AlignedPtr dest, Arg &&src)
    {
        using layout = typename Zipped::layout;
        using mapping = typename Zipped::mapping;

        return write_inline<layout, mapping>(dest, std::forward<Arg>(src));
    }

    template <typename Zipped, typename AlignedPtr, typename Arg>
    auto NETSER_FORCE_INLINE read_zipped_inline(AlignedPtr src, Arg &&dest)
    {
        using layout = typename Zipped::layout;
        using mapping = typename Zipped::mapping;

        return read_inline<layout, mapping>(src, std::forward<Arg>(dest));
    }

    template <typename PtrType, size_t PtrAlignment, size_t PtrDefect, typename PtrOffsetRange, typename Arg>
    auto operator>>(aligned_ptr<PtrType, PtrAlignment, PtrDefect, PtrOffsetRange> ptr, Arg &&arg)
    {
        return read_zipped_inline<decltype(default_zipped(std::declval<Arg>()))>(ptr, std::forward<Arg>(arg));
    }

    template <typename PtrType, size_t PtrAlignment, size_t PtrDefect, typename PtrOffsetRange, typename Arg>
    auto operator<<(aligned_ptr<PtrType, PtrAlignment, PtrDefect, PtrOffsetRange> ptr, Arg &&arg)
    {
        return write_zipped_inline<decltype(default_zipped(std::declval<Arg>()))>(ptr, std::forward<Arg>(arg));
    }

    template <typename T>
    using default_zipped_t
        = decltype(default_zipped(std::declval<T>())); // If this fails to compile you forgot to provide an overload of default_zipped(T)

    // fill_mapping_random
    // helper function for default_zipped mappings
    //
    //
    template <typename T>
    void fill_random(T &&arg)
    {
        using zipped_type = decltype(default_zipped(arg));
        using layout_type = typename zipped_type::layout::begin;
        using mapping_type = typename zipped_type::mapping;

        auto it = make_mapping_iterator<mapping_type>(arg);
        fill_mapping_random<layout_type>(it);
    }

} // namespace netser

#endif
