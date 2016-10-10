#ifndef NETSER_TEST_SHARED_HPP__
#define NETSER_TEST_SHARED_HPP__

#include <iostream>
#include <vector>

#define NETSER_DEREFERENCE_LOGGING
#include <netser/netser.hpp>
#include <netser/aligned_ptr.hpp>

class print_logger : public netser::dereference_logger
{
public:
    void log( uintptr_t memory_location, int offset, std::string type_name, size_t type_size, size_t type_alignment ) override
    {
        std::cout << "Access at " << std::hex << memory_location << " (+" << offset << "): " << type_name << " (size: " << type_size << ", align: " << type_alignment << ")\n";
    }
};

class collect_logger : public netser::dereference_logger
{
public:
    struct item {
        int offset;
        size_t size;
    };

    void log( uintptr_t memory_location, int offset, std::string type_name, size_t type_size, size_t type_alignment ) override
    {
        items_.push_back({ offset, type_size });
    }

    item& operator[]( size_t index )
    {
        return items_[index];
    }

    const item& operator[]( size_t index ) const
    {
        return items_[index];
    }

    size_t size() const
    {
        return items_.size();
    }

    void clear()
    {
        items_.clear();
    }

    std::vector< item > items_;
};

#endif
