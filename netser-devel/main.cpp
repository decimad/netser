#define NETSER_DEBUG_CONSOLE
#define NETSER_DEREFERENCE_LOGGING
#include <netser/netser.hpp>
#include <netser/range.hpp>
#include <merge_sort.hpp>
#include <string>
#include <cassert>

using netser::type_list;
using netser::next_t;
using netser::deref_t;

class print_logger : public netser::dereference_logger
{
public:
    void log( uintptr_t memory_location, int offset, std::string type_name, size_t type_size, size_t type_alignment ) override
    {
        std::cout << "Access at " << std::hex << memory_location << " (+" << offset << "): " << type_name << " (size: " << type_size << ", align: " << type_alignment << ")\n";
    }
};

int type_list_test()
{
    using range  = type_list< int, short, float >;

    static_assert(std::is_same< deref_t< range >, int >::value, "Wrong result");
    static_assert(std::is_same< deref_t<next_t<range>>, short >::value, "Wrong result");
    static_assert(std::is_same< deref_t<next_t<next_t<range>>>, float >::value, "Wrong result");
    static_assert(next_t<next_t<next_t<range>>>::empty(), "Wrong result" );

    netser::deref_t< range::begin > begin_val;
    netser::deref_t< netser::next_t< range > > second_val;

    return 0;
}

template< typename T >
struct print_typeid
{
    void operator()()
    {
        std::cout << typeid(T).name() << "\n";
    }
};

template< typename T >
struct print_value
{
    void operator()()
    {
        std::cout << T::value << "\n";
    }
};


int concat_range_test()
{
    using list1 = type_list< int, short, float, char, int, double >;
    using list2 = type_list< short, list1 >;

    using range = netser::ct::concat_range< list1, list2 >;
    print_typeid<deref_t<range>>()();

    std::cout << "Range for_each:\n";
    netser::ct::for_each< print_typeid >( range() );
    std::cout << "\n";

    std::cout << "Iterator for_each:\n";
    netser::ct::for_each< print_typeid >( range::begin(), range::end() );
    std::cout << "\n";

    return 0;
}

int test_integral_range()
{
    using range1 = netser::ct::integral_range<int,  0, 10>;
    using range2 = netser::ct::integral_range<int, 10, 20>;


    netser::ct::for_each< print_value >( netser::ct::concat_range< range1, range2 >() );
    return 0;
}


template< typename A, typename B >
struct less
{
    static constexpr bool value = (A::value < B::value);
};


void test_align_filter()
{
    using ptr_type = netser::aligned_ptr<void*, 4, 1, netser::two_side_limit_range<-1, 5>>;

    using filtered_list = netser::filtered_accesses_t< ptr_type, 2, netser::platform_memory_accesses >;
    std::cout << typeid( filtered_list ).name() << "\n";

/*
    constexpr size_t size = netser::detail::discover_write_span_size<
        netser::layout<
        >::begin
    >::value;
*/
}



int main()
{
    test_align_filter();

    unsigned int source = 15 << 24;
    unsigned int dest = 0;

    using layout  = netser::layout< netser::net_uint<32> >;
    using mapping = netser::mapping_list< netser::identity_member >;

    print_logger logger;

    auto ptr = netser::make_aligned_ptr<4>(&source, &logger);

    netser::read<layout, mapping>( ptr, dest );

    assert( dest == 15 );

    concat_range_test();
    test_integral_range();

}
