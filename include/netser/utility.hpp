//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_UTILITY_HPP__
#define NETSER_UTILITY_HPP__

#include <netser/type_list.hpp>

namespace netser {

    //
    // min/max
    //
    template< typename T >
    constexpr T min(T a, T b)
    {
        return (a < b) ? a : b;
    }

    template< typename T >
    constexpr T max(T a, T b)
    {
        return (a > b) ? a : b;
    }

    template< typename... Elements >
    struct dereference_stack {

        template< typename Element >
        using push = dereference_stack< Element, Elements... >;

        using pop = error_type;

        static constexpr size_t size = sizeof...(Elements);


        template< typename Type >
        static auto&& dereference_bottom_top(Type&& ref) {
            return std::forward<Type>(ref);
        }

        template< typename Type >
        static auto&& dereference_top_bottom(Type&& ref) {
            return std::forward<Type>(ref);
        }

    };

    template< typename Element0, typename... Elements >
    struct dereference_stack< Element0, Elements... >
    {
        using pop = dereference_stack< Elements... >;

        template< typename Element >
        using push = dereference_stack< Element, Element0, Elements... >;

        static constexpr size_t size = sizeof...(Elements)+1;

        template< typename Type >
        static auto&& dereference_bottom_top(Type&& ref)
        {
            return Element0::template dereference(pop::template dereference_bottom_top(std::forward<Type>(ref)));
        }

        template< typename Type >
        static auto&& dereference_top_bottom(Type&& ref)
        {
            return pop::template dereference_top_bottom(Element0::template dereference(std::forward<Type>(ref)));
        }

    };

    template< typename From, typename To >
    using copy_constness_t = std::conditional_t< std::is_const<From>::value, std::add_const_t<To>, std::remove_const_t<To> >;

    template< typename CtIterator >
    using next_t = typename CtIterator::advance;

    template< typename CtIterator >
    using deref_t = typename CtIterator::dereference;

    namespace detail {

        namespace impl {

            template< size_t Count, typename CtIteratorType >
            struct advance_ct_struct {
                using type = typename advance_ct_struct< Count - 1, next_t<CtIteratorType> >::type;
            };

            template< typename CtIteratorType >
            struct advance_ct_struct< 0, CtIteratorType >
            {
                using type = CtIteratorType;
            };

        }

    }

    // advance the compile-time-iterator CdIteratorType by Count elements
    //
    template< size_t Count, typename CdIteratorType >
    using advance_ct = typename detail::impl::advance_ct_struct< Count, CdIteratorType >::type;

    //
    // Execute body for every element of a compile-time-iterator-range
    //
    template< typename Begin, typename End, typename Current = Begin >
    struct for_interval_ct {
        template< template< typename Iterator > class Body, typename... Args >
        static void execute(Args&&... args) {
            Body< deref_t<Current> >::execute(std::forward<Args>(args)...);
            for_interval_ct< Begin, End, next_t<Current> >::template execute<Body>(std::forward<Args>(args)...);
        }
    };

    template< typename Begin, typename End >
    struct for_interval_ct< Begin, End, End > {
        template< template< typename Iterator > class Body, typename... Args >
        static void execute(Args&&... args) {
        }
    };

    //
    // Execute body for every element of a compile-time-container (range = container::begin to container::end)
    //
    template< typename Container >
    struct for_each_ct {
        template< template< typename Dereferenced > class Body, typename... Args >
        static void execute(Args&&... args) {
            for_interval_ct< typename Container::begin, typename Container::end >::template execute< Body >(std::forward<Args>(args)... );
        }
    };

    //
    // bit_range
    //

    template< int Begin, int End >
    struct bit_range
    {
        static_assert(End >= Begin, "Bad Range!");

        static constexpr int begin = Begin;
        static constexpr int end = End;
        static constexpr size_t size = end - begin;
    };

    template< typename BitRangeA, typename BitRangeB >
    using intersection = bit_range<
        max<int>(BitRangeA::begin, BitRangeB::begin),
        max<int>(min<int>(BitRangeA::end, BitRangeB::end), max<int>(BitRangeA::begin, BitRangeB::begin))
    >;

    // Cannot call it difference, since we don't support splitting into multiple intervals.
    template< typename A, typename B >
    using front_remove = bit_range< B::end, max<int>(A::end, B::end) >; // placement of empty result might be off!

    template< typename T >
    constexpr T bit_mask(size_t bits)
    {
        return (bits == sizeof(T)*8) ? T(-1) : static_cast<T>((T(1) << bits) - T(1));
    }

}

#endif
