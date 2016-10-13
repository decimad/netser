#define NETSER_DEBUG_CONSOLE
#define NETSER_DEREFERENCE_LOGGING
#include <netser/netser.hpp>
#include <netser/range.hpp>
#include <merge_sort.hpp>
#include <string>
#include <cassert>
#include <netser/fill_random.hpp>
#include <netser/static_array.hpp>

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

int main()
{
}
