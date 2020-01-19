//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_ZIPPED_HPP__
#define NETSER_ZIPPED_HPP__

#include <netser/integer.hpp>
#include <netser/layout.hpp>
#include <netser/mapping.hpp>


namespace netser
{

    template <typename... Args>
    struct zipped;

    namespace detail
    {

        template <typename Zipped, typename Class, typename Type, Type(Class::*Pointer)>
        struct zipped_member
        {
            using layout = typename Zipped::layout;
            using mapping = mapped_member<Class, Type, Pointer, typename Zipped::mapping>;
        };

        template <typename Layout, typename Mapping, typename... Args>
        struct unzip
        {
            using layout = Layout;
            using mapping = Mapping;
        };

        template <typename Layout, typename Mapping, typename LayoutElement, typename MappingElement, typename... Tail>
        struct unzip_pair : public unzip<push_back_t<Layout, LayoutElement>, push_back_t<Mapping, MappingElement>, Tail...>
        {
        };

        // Partial specialization for a <layout, struct-member-mapping> pair.
        template <typename Layout, typename Mapping, typename LayoutArg, typename... Tail>
        struct unzip<Layout, Mapping, LayoutArg, Tail...> : public unzip_pair<Layout, Mapping, LayoutArg, Tail...>
        {
        };

        // Nested zipped-member specialization
        template <typename... LayoutArgs, typename... MappingArgs, typename Zipped, typename Class, typename Type, Type(Class::*Pointer),
                  typename... Tail>
        struct unzip<layout<LayoutArgs...>, mapping_list<MappingArgs...>, zipped_member<Zipped, Class, Type, Pointer>, Tail...>
            : public unzip<layout<LayoutArgs..., typename zipped_member<Zipped, Class, Type, Pointer>::layout>,
                           mapping_list<MappingArgs..., typename zipped_member<Zipped, Class, Type, Pointer>::mapping>, Tail...>
        {
        };

        // "reserved" partial specialization (-> expands to a <net_uint, constant=0> pair).
        template <typename... LayoutArgs, typename... MappingArgs, size_t Bits, typename... Tail>
        struct unzip<layout<LayoutArgs...>, mapping_list<MappingArgs...>, reserved<Bits>, Tail...>
            : public unzip<layout<LayoutArgs..., net_uint<Bits>>, mapping_list<MappingArgs..., constant<size_t, 0>>, Tail...>
        {
        };

        // Nested zipped partial specialization
        template <typename... LayoutArgs, typename... MappingArgs, typename... ZippedArgs, typename... Tail>
        struct unzip<layout<LayoutArgs...>, mapping_list<MappingArgs...>, zipped<ZippedArgs...>, Tail...>
            : public unzip<layout<LayoutArgs..., typename zipped<ZippedArgs...>::layout>,
                           mapping_list<MappingArgs..., typename zipped<ZippedArgs...>::mapping>, Tail...>
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

// I will be so happy once C++17 allows for auto value arguments.
#define ZIPPED_MEMBER(MemberAccess)                                                                                                        \
    ::netser::detail::zipped_member<::netser::default_zipped_t<decltype(::netser::detail::deduce_type(&MemberAccess))>,                    \
                                    decltype(::netser::detail::deduce_class(&MemberAccess)),                                               \
                                    std::remove_reference_t<decltype(::netser::detail::deduce_type(&MemberAccess))>, &MemberAccess>

#endif
