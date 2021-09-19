#include <netser/mapping.hpp>

void test_mapping()
{
    {
        using mapping = netser::mapping_list<netser::identity>;
        size_t val;
        auto iter1 = netser::make_mapping_iterator<mapping>(val);

        static_assert(std::is_same_v<decltype(iter1.dereference()), size_t&>, "Error!");
    }

    {
        struct foo {
            size_t value;
            float   character;
        };
        foo val;

        using mapping = netser::mapping_list<netser::mem<&foo::value>, netser::mem<&foo::character>>;
        auto iter = netser::make_mapping_iterator<mapping>(val);

        using pathrange = decltype(iter)::path_enumerator;

        static_assert(std::is_same_v<decltype(iter.dereference()), size_t&>, "Error!");

        auto iter2 = iter.advance();
        static_assert(std::is_same_v<decltype(iter2.dereference()), float&>, "Error!");
    }
}