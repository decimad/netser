//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_LAYOUT_MAPPING_HPP__
#define NETSER_LAYOUT_MAPPING_HPP__

#include <netser/utility.hpp>
#include <type_traits>


namespace netser
{

    template <typename T>
    constexpr bool is_empty_mapping()
    {
        return T::size == 0;
    }

    template <typename Member, typename RefStack>
    struct mapping_sequence_value_type
    {
        using member = Member;
        using refstack = RefStack;

        template <typename Arg>
        static auto &&dereference(Arg &&arg)
        {
            return refstack::dereference_top_bottom(arg);
        }
    };

    template <typename Mapping, size_t Size>
    struct mapping_iterator_ct
    {
      private:
        using pop_result = typename Mapping::pop;
        using member = typename pop_result::member;
        using tail = typename pop_result::tail_mapping;
        using refstack = typename pop_result::refstack;

      public:
        using dereference = mapping_sequence_value_type<member, refstack>;
        using advance = mapping_iterator_ct<tail, tail::size>;

        static constexpr bool is_end = false;
    };

    template <typename Mapping>
    struct mapping_iterator_ct<Mapping, 0>
    {
        using dereference = error_type;
        using advance = error_type;

        static constexpr bool is_end = true;
    };

    template <typename MappingIteratorCt, typename Arg, bool IsEnd = MappingIteratorCt::is_end>
    struct mapping_iterator
    {
      public:
        using ct_iterator = MappingIteratorCt;

        constexpr mapping_iterator(Arg arg) : arg_(arg)
        {
        }

        constexpr auto &&dereference() const
        {
            using refstack = typename deref_t<ct_iterator>::refstack;
            return refstack::dereference_top_bottom(arg_);
        }

        constexpr auto getval() const
        {
            using refstack = typename deref_t<ct_iterator>::refstack;
            return refstack::dereference_top_bottom(arg_);
        }

        constexpr auto advance() const
        {
            return mapping_iterator<next_t<ct_iterator>, Arg>(arg_);
        }

        static constexpr bool is_end = ct_iterator::is_end;

        Arg arg_;
    };

    template <typename MappingIteratorCt, typename RefType>
    struct mapping_iterator<MappingIteratorCt, RefType, true>
    {
      public:
        using ct_iterator = MappingIteratorCt;

        constexpr mapping_iterator(RefType &ref) : ref_(ref)
        {
        }

        constexpr auto dereference() const
        {
            return error_type();
        }

        constexpr auto advance() const
        {
            return error_type();
        }

        static constexpr bool is_end = ct_iterator::is_end;

        RefType &ref_;
    };

    template <typename Mapping, typename Arg>
    constexpr auto make_mapping_iterator(Arg &&arg) -> mapping_iterator<typename Mapping::begin, Arg>
    // cdt index parser cannont look at return statement in function body.
    {
        return mapping_iterator<typename Mapping::begin, Arg>(std::forward<Arg>(arg));
    }

    namespace detail
    {

        template <typename Leaf, typename TailMapping, typename DereferenceStack>
        struct mapping_pop_result
        {
            using member = Leaf;
            using tail_mapping = TailMapping;
            using refstack = DereferenceStack;
        };

    } // namespace detail

    // Inner node mapping interface:
    // General:
    // size_t member size  (aggregated number of leafs (true members))
    //
    // For iterator interface:
    // type member pop -> mapping_pop_result
    //
    // For random access:
    //
    //

    template <typename... Mappings>
    struct mapping_list
    {
      private:
        template <typename... List>
        struct get_size
        {
            static constexpr size_t size = 0;
        };

        template <typename Mapping0, typename... Remaining>
        struct get_size<Mapping0, Remaining...>
        {
            static constexpr size_t size = Mapping0::size + get_size<Remaining...>::size;
        };

      public:
        static constexpr size_t size = get_size<Mappings...>::size;

        using begin = mapping_iterator_ct<mapping_list, size>;
        using end = mapping_iterator_ct<mapping_list<>, 0>;

      private:
        template <typename... Types>
        struct pop_struct
        {
            using result = error_type;
        };

        template <typename Type0, typename... Tail>
        struct pop_struct<Type0, Tail...>
        {
            using nested_pop = typename Type0::pop;

            using result = std::conditional_t<
                nested_pop::tail_mapping::size == 0 /* empty mapping */,
                detail::mapping_pop_result<typename nested_pop::member, mapping_list<Tail...>, typename nested_pop::refstack>,
                detail::mapping_pop_result<typename nested_pop::member, mapping_list<typename nested_pop::tail_mapping, Tail...>,
                                           typename nested_pop::refstack>>;
        };

      public:
        using pop = typename pop_struct<Mappings...>::result;

      private:
        template <typename Type, size_t Offset>
        struct get_mapping_struct
        {
            using type = Type;
            static constexpr size_t offset = Offset;
        };

      public:
        template <size_t Index, size_t Current, typename... SearchMappings>
        struct get_mapping
        {
        };

        template <size_t Index, size_t Current, typename Remaining0, typename... Remaining>
        struct get_mapping<Index, Current, Remaining0, Remaining...>
            : public std::conditional_t<(Remaining0::size + Current > Index), get_mapping_struct<Remaining0, Index - Current>,
                                        get_mapping<Index, Current + Remaining0::size, Remaining...>>
        {
        };
    };

    template <typename Class, typename Type, Type Class::*Ptr, typename MappingsList>
    struct mapped_member
    {
        using base = MappingsList;

        static constexpr size_t size = base::size;

        using nested_pop = typename base::pop;
        using pop = detail::mapping_pop_result<typename nested_pop::member,
                                               std::conditional_t<is_empty_mapping<typename nested_pop::tail_mapping>(), mapping_list<>,
                                                                  mapped_member<Class, Type, Ptr, typename nested_pop::tail_mapping>>,
                                               typename nested_pop::refstack::template push<mapped_member>>;

        template <typename Arg>
        static auto &&dereference(Arg &&arg)
        {
            return arg.*Ptr;
        }
    };

    // identity_member
    // helper type (used f.e. by static_array layout)
    //
    struct identity_member
    {

        static constexpr size_t size = 1;
        using pop = detail::mapping_pop_result<identity_member, mapping_list<>, dereference_stack<>>;

        template <typename Arg>
        static auto &&dereference(Arg &&arg)
        {
            return std::forward<Arg>(arg);
        }
    };

    using identity = identity_member;

    template <typename T>
    struct enum_wrapper
    {
        enum_wrapper(T &anEnum) : enum_(anEnum)
        {
        }

        static_assert(std::is_enum<T>::value, "No enum type.");

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

    //
    // I'm not conviced yet it is an acceptable hack to return an enum wrapper, but it is a
    // convenient solution to support enums for now.
    template <typename Class, typename Type, Type Class::*Ptr, bool IsEnum = std::is_enum<Type>::value>
    struct mem
    {
        static constexpr size_t size = 1;
        using value_type = Type;

        static constexpr Type Class::*pointer = Ptr;
        static constexpr const Type Class::*const_pointer = Ptr;

        using pop = detail::mapping_pop_result<mem, mapping_list<>, dereference_stack<mem>>;

        //
        //  dereference_stack interface
        //
        template <typename Ref>
        static Type &dereference(Ref &ref)
        {
            return ref.*pointer;
        }

        template <typename Ref>
        static const Type &dereference(const Ref &ref)
        {
            return ref.*const_pointer;
        }
    };

    template <typename Class, typename Type, Type Class::*Ptr>
    struct mem<Class, Type, Ptr, true>
    {
        static constexpr size_t size = 1;
        using value_type = Type;

        static constexpr Type Class::*pointer = Ptr;
        static constexpr const Type Class::*const_pointer = Ptr;

        using pop = detail::mapping_pop_result<mem, mapping_list<>, dereference_stack<mem>>;

        //
        //  dereference_stack interface
        //
        template <typename Ref>
        static enum_wrapper<Type> dereference(Ref &ref)
        {
            return {ref.*pointer};
        }

        template <typename Ref>
        static enum_wrapper<const Type> dereference(const Ref &ref)
        {
            return {ref.*const_pointer};
        }
    };

    namespace detail
    {

        template <typename Class, typename Type, size_t Size, size_t Offset, Type (Class::*Ptr)[Size]>
        struct exploded_array_member
        {
            static constexpr size_t size = Size - Offset;

            using pop = mapping_pop_result<exploded_array_member, exploded_array_member<Class, Type, Size, Offset + 1, Ptr>,
                                           dereference_stack<exploded_array_member>>;

            template <typename Arg>
            static auto &dereference(Arg &&arg)
            {
                return (static_cast<copy_constness_t<Arg, Class> &>(arg).*Ptr)[Offset];
            }

            template <size_t Index, typename Ref, typename Value>
            static void set(Ref &&ref, Value value)
            {
                static_assert(Offset + Index < Size, "Exceeding Array Bounds!");
                (ref.*Ptr)[Offset + Index] = value;
            }

            template <size_t Index, typename Ref>
            static Type get(Ref &&ref)
            {
                static_assert(Offset + Index < Size, "Exceeding Array Bounds!");
                return (ref.*Ptr)[Offset + Index];
            }
        };

    } // namespace detail

    template <typename Class, typename Type, size_t Size, Type (Class::*Ptr)[Size]>
    using array_member = detail::exploded_array_member<Class, Type, Size, 0, Ptr>;

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

            operator T() const
            {
                return value;
            }

            // ignore assignment
            template <typename Other>
            void operator=(Other) const
            {
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
        static constexpr size_t size = 1;
        using pop = detail::mapping_pop_result<constant, mapping_list<>, dereference_stack<constant>>;

        using value_type = T;

        template <typename... Arg>
        static detail::constant_reference<T, value> dereference(Arg &&...)
        {
            return detail::constant_reference<T, value>();
        }
    };

} // namespace netser

// I will be so happy once C++17 allows for auto value arguments.
#define MEMBER(Class, Member)                                                                                                              \
    ::netser::mem<Class, std::remove_reference_t<decltype(::netser::detail::deduce_type(&Class::Member))>, &Class::Member>
#define MEMBER1(MemberAccess)                                                                                                              \
    ::netser::mem<decltype(::netser::detail::deduce_class(&MemberAccess)),                                                                 \
                  std::remove_reference_t<decltype(::netser::detail::deduce_type(&MemberAccess))>, &MemberAccess>
#define MEMBER_ARRAY(Class, Member)                                                                                                        \
    ::netser::array_member<Class, decltype(::netser::detail::deduce_member_array_type(&Class::Member)),                                    \
                           ::netser::detail::deduce_member_array_size(&Class::Member), &Class::Member>

#define NESTED(Class, Member, Mapping)                                                                                                     \
    ::netser::mapped_member<Class, decltype(::netser::detail::deduce_type(&Class::Member)), &Class::Member, Mapping>
#define NESTED_DEFAULT(Class, Member)                                                                                                      \
    ::netser::mapped_member<Class, decltype(::netser::detail::deduce_type(&Class::Member)), &Class::Member,                                \
                            decltype(default_mapping(std::declval<decltype(::netser::detail::deduce_type(&Class::Member))>()))>

#endif
