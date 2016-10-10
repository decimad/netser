//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_ALIGNED_PTR_HPP__
#define NETSER_ALIGNED_PTR_HPP__

#include <type_traits>
#include <netser/remainder.hpp>

#ifdef NETSER_DEBUG_CONSOLE
#include <iostream>
#include <cassert>
#endif

namespace netser {

#ifdef NETSER_DEREFERENCE_LOGGING
class dereference_logger {
public:
    virtual void log( uintptr_t memory_location, int offset, std::string type_name, size_t type_size, size_t type_alignment ) = 0;
};
#endif

template< typename Type, size_t Alignment, size_t Defect = 0, int MinOffset = 0 >
struct aligned_ptr {
    using value_type = Type;
    using type = Type* const;

    // Alignment + Defect are actually Alignment+Defect in GCC parlance, which form a residue class
    using residue = residue_class< Alignment, Defect >;

private:
    static constexpr size_t pointer_alignment = residue::alignment();
    static constexpr size_t max_alignment     = Alignment;

public:
    // get_pointer_alignment
    // Returns the true alignment of the stored pointer (before applying the static offset)
    static constexpr size_t get_pointer_alignment()
    {
        return pointer_alignment;
    }

    // get_pointer_alignment
    // Returns the alignment defect wrt. get_max_aligment of the pointer (before applying the static offset)
    static constexpr size_t get_pointer_defect()
    {
        return Defect;
    }

    // get_max_alignment
    // Returns the maximum alignment that can be guaranteed with changing offsets
    static constexpr size_t get_max_alignment()
    {
        return Alignment;
    }

    // get_access_alignment
    // Returns the aligment of an access (after passed offset)
    static constexpr size_t get_access_alignment( int offset_bytes = 0 )
    {
        return residue::offset_alignment( offset_bytes );
    }

    // align_down
    // Reduces the given offset so that an access with the offset meets the alignment requirement RequestedAlignment
    static constexpr int align_down( int Offset, size_t RequestedAlignment )
    {
        return residue::align_down( Offset, RequestedAlignment );
    }

    constexpr aligned_ptr( Type* ptr
#ifdef NETSER_DEREFERENCE_LOGGING
        , dereference_logger* logger = nullptr
#endif
    )
        : ptr_( ptr )
#ifdef NETSER_DEREFERENCE_LOGGING
        , logger_( logger )
#endif
    {
#ifdef NETSER_DEBUG_CONSOLE
        assert( reinterpret_cast<uintptr_t>(ptr) % Alignment == Defect );
#endif
    }

    constexpr aligned_ptr( const aligned_ptr& ) = default;
    constexpr aligned_ptr( aligned_ptr&& ) = default;

    template< int Offset >
    constexpr std::remove_const_t<type> get_offset() const {
        static_assert( Offset >= MinOffset, "Getting pointer to invalid memory location." );

#ifdef __GNUC__
        // We can hint the alignment+defect, maybe GCC can put it to use
        return reinterpret_cast<type>(__builtin_assume_aligned(
            reinterpret_cast<copy_constness_t<Type, char>* const>(ptr_) + Offset,
            residue::template offset_class<Offset>::divisor,
            residue::template offset_class<Offset>::remainder
        ));
#else
        return reinterpret_cast<type>(reinterpret_cast<copy_constness_t<Type, char>* const>(ptr_) + Offset);
#endif
    }

    constexpr std::remove_const_t<type> get() const {
        return get_offset<0>();
    }

    template< typename T, int Offset = 0 >
    T& dereference() const
    {
#ifdef NETSER_DEREFERENCE_LOGGING
        if( logger_ ) {
            logger_->log(
                reinterpret_cast< uintptr_t >( get_offset<0>() ),
                Offset,
                typeid(T).name(),
                sizeof(T),
                alignof(T)
            );
        }
#endif
        return *reinterpret_cast<T*>(reinterpret_cast<copy_constness_t<Type, char>*>(get_offset<Offset>()));
    }

    // static_offset_bits
    // Convenience checked offset.
    // Offset this pointer by a static amount of bits. RelativeOffset must be a multiple of 8 bits.
    template< size_t RelativeOffset >
    constexpr auto static_offset_bits() const
    {
        return static_offset< RelativeOffset/8 >();
    }

    template< int Offset >
    using static_offset_t = aligned_ptr<
        Type,
        residue::template offset_class<Offset>::divisor,
        residue::template offset_class<Offset>::remainder,
        MinOffset
    >;

    // static_offset
    // Offset this pointer by a static amount of bytes.
    //
    template< int Offset >
    static_offset_t<Offset> static_offset() const
    {
        return static_offset_t<Offset>( get_offset<Offset>()
#ifdef NETSER_DEREFERENCE_LOGGING
            , logger_
#endif
            );
    }

    // stride
    // Offset this pointer by a dynamic amount of static strides in bytes.
    template< size_t StrideBytes >
    auto stride( size_t index ) const
    {
        return aligned_ptr<
            Type, gcd( get_access_alignment( 0 ), power2_alignment_of( StrideBytes ) )
        >( reinterpret_cast<Type*>(reinterpret_cast<copy_constness_t<Type, char>*>(ptr_) + StrideBytes*index)
#ifdef NETSER_DEREFERENCE_LOGGING
            , logger_
#endif
            );
    }

    // stride_offset
    template< size_t StrideBytes, size_t RelativeOffsetBytes >
    auto stride_offset( size_t index ) {
        return static_offset< RelativeOffsetBytes >().template stride< StrideBytes >( index );
    }

#ifdef NETSER_DEBUG_CONSOLE
    void describe()
    {
        std::cout << "Alignment: " << pointer_alignment;
        if( Defect != 0 ) std::cout << "(+" << Defect << ")";
    }
#endif

    type ptr_;

#ifdef NETSER_DEREFERENCE_LOGGING
    dereference_logger* logger_;
#endif
};

template< size_t Alignment, int Offset = 0, size_t Defect = 0, int MinOffset = 0, typename Arg >
auto make_aligned_ptr( Arg* arg
#ifdef NETSER_DEREFERENCE_LOGGING
    , dereference_logger* logger = nullptr
#endif
)
#ifndef _MSC_VER    // Hide IntelliSense-Error
-> typename aligned_ptr< Arg, Alignment, Defect, MinOffset >::template static_offset_t<Offset>
#endif
{
    return aligned_ptr< Arg, Alignment, Defect, MinOffset >( arg, logger ).template static_offset<Offset>();
}

template< int Offset, size_t Alignment, size_t Defect, int MinOffset, typename Type >
auto offset( aligned_ptr<Type, Alignment, Defect, MinOffset> ptr )
{
    return ptr.template static_offset<Offset>();
}

}

#endif

