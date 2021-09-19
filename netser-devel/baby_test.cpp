#include "netser/aligned_ptr.hpp"
#include "netser/mapping.hpp"
#include <cstddef>
#include <netser/netser.hpp>
#include <meta/range.hpp>
#include <array>

// -- Simple test ----------------------------------

namespace simple {

    struct my_struct {
        size_t member;
    };

    using netser::net_uint;
    using netser::mem;

    using my_struct_zipped = netser::zipped<
        net_uint<32>, mem<&my_struct::member>
    >;

    my_struct_zipped default_zipped(my_struct&);

    char buffer[128];
    my_struct data;

    void test()
    {
        netser::make_aligned_ptr<1>(buffer) >> data;
        netser::make_aligned_ptr<1>(buffer) << data;

        netser::make_aligned_ptr<2>(buffer) >> data;
        netser::make_aligned_ptr<2>(buffer) >> data;

        netser::make_aligned_ptr<4>(buffer) << data;
        netser::make_aligned_ptr<4>(buffer) << data;
    }

}

// -- Small array test ----------------------------

namespace small_array {

    char buffer[128];

    std::array<unsigned char, 7> my_array;
    char builtin_array[7];

    using netser::net_uint;

    using zipped = netser::zipped<
        net_uint<8>[7], netser::identity
    >;

    void test()
    {
        auto ptr1 = netser::make_aligned_ptr<1>(buffer);
        auto ptr2 = netser::make_aligned_ptr<2>(buffer);
        auto ptr4 = netser::make_aligned_ptr<4>(buffer);

        // read_inline<netser::layout<net_uint<8>[7]>, netser::identity>(ptr1, builtin_array);
    }

}

// -- Large array test ----------------------------

namespace large_array {

    void large_array_test()
    {

    }

}


void baby_test()
{
    simple::test();
    small_array::test();
}
