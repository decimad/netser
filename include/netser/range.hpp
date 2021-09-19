//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_RANGE_HPP__
#define NETSER_RANGE_HPP__

#include "meta/common.hpp"
#include "netser/utility.hpp"
#include <meta/tlist.hpp>
#include <meta/range.hpp>


namespace netser
{

    // fixme -->   add to meta!

    // while_
    //
    //
    template <meta::concepts::Enumerator Range, template <typename> class Condition, template <typename, typename> class Body, typename State>
    struct while_;

    namespace detail
    {

        template <typename Range, template <typename> class Condition, template <typename, typename> class Body, typename State,
                  bool Finish = !Condition<meta::dereference_t<Range>>::value>
        struct while_inner
        {
            using type = typename while_<meta::advance_t<Range>, Condition, Body, typename Body<meta::dereference_t<Range>, State>::type>::type;
        };

        template <typename Range, template <typename> class Condition, template <typename, typename> class Body, typename State>
        struct while_inner<Range, Condition, Body, State, true>
        {
            using type = State;
        };

    } // namespace detail

    template <meta::concepts::Enumerator Range, template <typename> class Condition, template <typename, typename> class Body, typename State>
    struct while_
    {
        using type = typename detail::while_inner<Range, Condition, Body, State>::type;
    };

    template <meta::concepts::Sentinel Range, template <typename> class Condition, template <typename, typename> class Body, typename State>
    struct while_<Range, Condition, Body, State>
    {
        using type = State;
    };
} // namespace netser

#endif
