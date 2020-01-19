//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_REMAINDER_HPP__
#define NETSER_REMAINDER_HPP__

#include <cstddef>

namespace netser
{

    using std::size_t;
    constexpr size_t gcd(size_t a, size_t b)
    {
        return (b == 0) ? a : gcd(b, a % b);
    }

    constexpr size_t unsigned_mod(int val, size_t divisor)
    {
        return (val < 0) ? (divisor - (val % divisor)) : val % divisor;
    }

    template <size_t Divisor, size_t Remainder>
    struct residue_class
    {

        static constexpr size_t divisor = Divisor;
        static constexpr size_t remainder = Remainder;

        static constexpr size_t alignment()
        {
            return gcd(Divisor, Remainder);
        }

        // Returns the remainder after offsetting this class by "offset".
        static constexpr size_t offset_remainder(int offset)
        {
            return (Remainder % Divisor + unsigned_mod(offset, Divisor)) % Divisor;
        }

        // Returns the alignment after offsetting this class by "offset".
        static constexpr size_t offset_alignment(int offset)
        {
            return gcd(Divisor, offset_remainder(offset));
        }

        template <int Offset>
        using offset_class = residue_class<Divisor, offset_remainder(Offset)>;

        // Returns the next smaller Offset that has the alignment Alignment
        static constexpr int align_down(int Offset, size_t Alignment)
        {
            return Offset - offset_remainder(Offset) % Alignment;
        }
    };

    constexpr size_t power2_alignment_of(size_t value)
    {
        return value == 0 ? 1 : ((value % 2 == 0) ? 2 * power2_alignment_of(value / 2) : 1);
    }

    /*
    inline void test_remainder() {

        using type = residue_class< 4, 2 >;

        constexpr size_t alignment = type::alignment();
        constexpr size_t off       = type::offset_class<53>::remainder; //offset_alignment(53);

        constexpr int aligned   = type::align_down(2, 4);

    }
    */

} // namespace netser

#endif
