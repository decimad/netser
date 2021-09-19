//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_LAYOUT_MAPPING_HPP__
#define NETSER_LAYOUT_MAPPING_HPP__

#include <meta/tree.hpp>
#include <netser/utility.hpp>
#include <type_traits>

namespace netser
{

    template<typename Mapping>
    using mapping_begin_t = meta::tree_begin<Mapping, meta::contexts::intrusive, meta::traversals::lr>;

    template<typename Mapping>
    using mapping_end_t   = meta::tree_end<Mapping, meta::contexts::intrusive, meta::traversals::lr>;

    template<typename Mapping>
    using mapping_range_t = meta::tree_range_t<Mapping, meta::contexts::intrusive, meta::traversals::lr>;

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

    namespace concepts {

        template<auto T>
        concept MemberPtr = std::is_member_object_pointer_v<decltype(T)>;

    }

    template <meta::concepts::Enumerator MappingRange, typename Arg>
    struct mapping_iterator
    {
      public:
        using path_enumerator = meta::path_enumerator_t<MappingRange>;

        static constexpr bool is_end = false;

        constexpr mapping_iterator(Arg arg) : arg_(arg)
        {
        }

        constexpr auto getval() const
        {
            return netser::dereference<path_enumerator>(arg_);
        }

        constexpr auto &operator*() const
        {
            return netser::dereference<path_enumerator>(arg_);
        }

        constexpr auto operator++() const
        {
            return mapping_iterator<meta::advance_t<MappingRange>, Arg>(arg_);
        }

        Arg arg_;
    };

    template <meta::concepts::Sentinel MappingRange, typename Arg>
    struct mapping_iterator<MappingRange, Arg>
    {
      public:
        using range = MappingRange;

        static constexpr bool is_end = true;

        constexpr mapping_iterator(Arg &ref [[maybe_unused]])
        {
        }

        constexpr auto operator*() const
        {
            return meta::error_type();
        }

        constexpr auto operator++() const
        {
            return meta::error_type();
        }
    };

    template <typename Mapping, typename Arg>
    constexpr auto make_mapping_iterator(Arg &&arg) /*-> mapping_iterator<mapping_range_t<Mapping>, Arg>*/
    // cdt index parser cannont look at return statement in function body.
    {
        return mapping_iterator<mapping_range_t<Mapping>, Arg>(std::forward<Arg>(arg));
    }

    template <typename... Mappings>
    struct mapping_list
    {
        static constexpr size_t num_children = sizeof...(Mappings);

        template<size_t Pos>
        using get_child = meta::type_list::get<meta::tlist<Mappings...>, Pos>;

        template <typename Arg>
        static auto &&apply(Arg &&arg)
        {
            return arg;
        }
    };


    template<auto MemberPtr>
    requires(std::is_member_object_pointer_v<decltype(MemberPtr)>)
    struct member_object_pointer_traits;

    template<typename Container, typename Type, Type Container::*Ptr>
    struct member_object_pointer_traits<Ptr>
    {
        using container_type = Container;
        using value_type = Type;
    };

    template<auto Ptr, typename MappingsList>
    requires(concepts::MemberPtr<Ptr>)
    struct mapped_member
    {
        static constexpr size_t num_children = meta::type_list::size<MappingsList>;

        template<size_t Pos>
        using get_child = meta::type_list::get<MappingsList, Pos>;

        template <typename Arg>
        static auto &&apply(Arg &&arg)
        {
            return arg.*Ptr;
        }
    };

    // identity_member
    // helper type (used f.e. by static_array layout)
    //
    struct identity_member
    {
        static constexpr size_t num_children = 0;

        template <typename Arg>
        static Arg apply(Arg &&arg)
        {
            return arg;
        }
    };

    using identity = identity_member;

    template <typename T>
    requires(std::is_enum_v<T>)
    struct enum_wrapper
    {
        enum_wrapper(T &anEnum) : enum_(anEnum)
        {
        }

        using underlying_type = std::underlying_type_t<T>;

        template <typename O>
        enum_wrapper &operator=(O &&val)
        {
            enum_ = static_cast<T>(val);
            return *this;
        }

        operator underlying_type() const
        {
            return static_cast<underlying_type>(enum_);
        }

        T &enum_;
    };

    template<auto MemberPtr, bool IsEnum = std::is_enum_v<decltype(MemberPtr)>>
    requires(concepts::MemberPtr<MemberPtr>)
    struct mem
    {
        using member_info = member_object_pointer_traits<MemberPtr>;

        static constexpr size_t num_children = 0;

        using value_type     = typename member_info::value_type;
        using container_type = typename member_info::container_type;

        static constexpr value_type container_type::*pointer = MemberPtr;
        static constexpr const value_type container_type::*const_pointer = MemberPtr;

        //
        //  dereference_stack interface
        //
        template <typename Ref>
        static value_type &apply(Ref &ref)
        {
            return ref.*pointer;
        }

        template <typename Ref>
        static const value_type &apply(const Ref &ref)
        {
            return ref.*const_pointer;
        }
    };

    template<auto MemberPtr>
    requires(concepts::MemberPtr<MemberPtr>)
    struct mem<MemberPtr, true>
    {
        using member_info = member_object_pointer_traits<MemberPtr>;

        static constexpr size_t num_children = 0;

        using value_type     = typename member_info::value_type;
        using container_type = typename member_info::container_type;

        static constexpr value_type container_type::*pointer = MemberPtr;
        static constexpr const value_type container_type::*const_pointer = MemberPtr;

        template <typename Ref>
        static enum_wrapper<value_type> dereference(Ref &ref)
        {
            return {ref.*pointer};
        }

        template <typename Ref>
        static enum_wrapper<const value_type> dereference(const Ref &ref)
        {
            return {ref.*const_pointer};
        }
    };

    namespace detail
    {

        template<size_t Pos>
        struct array_indexer {

            template<typename T>
            static auto &apply(T &ref)
            {
                return ref[Pos];
            }

        };


        template <typename Class, typename Type, size_t Size, Type (Class::*Ptr)[Size]>
        struct exploded_array_member
        {
            static constexpr size_t num_children = Size;

            template<size_t Pos>
            using get_child = array_indexer<Pos>;

            template <typename Ref>
            static auto &apply(Ref &ref)
            {
                return ref.*Ptr;
            }
        };

    } // namespace detail

    template <typename Class, typename Type, size_t Size, Type (Class::*Ptr)[Size]>
    using array_member = detail::exploded_array_member<Class, Type, Size, Ptr>;

    namespace detail
    {

        //
        // Macro helper functions
        //

        // Deduce class type from pointer-to-class-member
        template <typename Class, typename Type>
        Class deduce_class(Type Class::*ptr);

        // Deduce member value type from pointer-to-class-member
        template <typename Class, typename Type>
        Type deduce_type(Type Class::*ptr);

        template <typename Class, typename Type, size_t Size>
        Type (&deduce_type(Type (Class::*)[Size]))[Size];

        // Deduce value type from pointer-to-class-member-array
        template <typename Class, typename Type, size_t Size>
        Type deduce_member_array_type(Type (Class::*)[Size]);

        // Deduce array size from pointer-to-class-member-array
        template <typename Class, typename Type, size_t Size>
        constexpr size_t deduce_member_array_size(Type (Class::*)[Size])
        {
            return Size;
        }

    } // namespace detail

    namespace detail
    {

        template <typename T, T value>
        struct constant_reference
        {
            constant_reference()
            {}

            constant_reference(T)
            {}

            operator T() const
            {
                return value;
            }

            constant_reference& operator=(constant_reference&)
            {
                return *this;
            }

            // ignore assignment
            template <typename Other>
            constant_reference& operator=(Other)
            {
                return *this;
            }

            // index to self
            constant_reference operator[](size_t index)
            {
                return constant_reference();
            }

            decltype(value >> size_t()) operator>>(size_t val)
            {
                return value >> val;
            }

            decltype(value << size_t()) operator<<(size_t val)
            {
                return value << val;
            }

            template <typename MaskType>
            decltype(value & MaskType()) operator&(MaskType mask)
            {
                return value & mask;
            }

            template <typename MaskType>
            decltype(value | MaskType()) operator|(MaskType mask)
            {
                return value | mask;
            }

            template <typename MaskType>
            decltype(value ^ MaskType()) operator^(MaskType mask)
            {
                return value ^ mask;
            }
        };

    } // namespace detail

    template <typename T, T value = 0>
    struct constant
    {
        static constexpr size_t num_children = 0;

        using value_type = T;

        template <typename... Arg>
        static detail::constant_reference<T, value> apply(Arg &&...)
        {
            return detail::constant_reference<T, value>();
        }
    };

} // namespace netser

// I will be so happy once C++17 allows for auto value arguments.
#define MEMBER_ARRAY(Class, Member)                                                                                                        \
    ::netser::array_member<Class, decltype(::netser::detail::deduce_member_array_type(&Class::Member)),                                    \
                           ::netser::detail::deduce_member_array_size(&Class::Member), &Class::Member>

#define NESTED(Class, Member, Mapping)                                                                                                     \
    ::netser::mapped_member<Class, decltype(::netser::detail::deduce_type(&Class::Member)), &Class::Member, Mapping>
#define NESTED_DEFAULT(Class, Member)                                                                                                      \
    ::netser::mapped_member<Class, decltype(::netser::detail::deduce_type(&Class::Member)), &Class::Member,                                \
                            decltype(default_mapping(std::declval<decltype(::netser::detail::deduce_type(&Class::Member))>()))>

#endif
