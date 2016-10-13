//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_STATIC_ARRAY_HPP__
#define NETSER_STATIC_ARRAY_HPP__

#include <netser/layout.hpp>

namespace netser {

    namespace detail {

        template< size_t Index, size_t End, typename Field >
        struct unroll {
            template< typename ZipIterator >
            static NETSER_FORCE_INLINE auto read( ZipIterator it )
            {
                constexpr size_t offset = ZipIterator::layout_iterator::get_offset();
                read_inline< layout<Field>, mapping_list<identity_member> >( it.layout().get().template stride_offset<Field::size / 8, offset / 8>( Index ), it.mapping().dereference()[Index] );
                return unroll< Index+1, End, Field >::template read(it);
            }


            template< typename ZipIterator >
            static NETSER_FORCE_INLINE auto write( ZipIterator it )
            {
                constexpr size_t offset = ZipIterator::layout_iterator::get_offset();
                write_inline< layout<Field>, mapping_list<identity_member> >( it.layout().get().template stride_offset<Field::size / 8, offset / 8>( Index ), it.mapping().dereference()[Index] );
                return unroll< Index+1, End, Field >::template write(it);
            }



        };

        template< size_t End, typename Field >
        struct unroll< End, End, Field >
        {
            template< typename ZipIterator >
            static NETSER_FORCE_INLINE auto write( ZipIterator it )
            {
                return it.advance();
            }

            template< typename ZipIterator >
            static NETSER_FORCE_INLINE auto read( ZipIterator it )
            {
                return it.advance();
            }
        };

    }

    // array_layout
    // layout of static sized array.
    // Note: Upon reading/writing to/from a mapping, that mapping must dereference to something that is indexable
    template< typename Field, size_t Size, size_t UnrollMax >
    struct array_layout : public detail::simple_field_layout_mixin< array_layout< Field, Size, UnrollMax > >
    {
        static_assert(Field::size % 8 == 0, "Unsupported Array Element Stride!");

        static constexpr size_t count = 1;
        static constexpr size_t size = Field::size * Size;


        struct identity_mapping {

            static constexpr size_t count = 1;

            template< typename Arg >
            Arg dereference(Arg&& arg)
            {
                return arg;
            }

        };

        // Fixme: Add a fast path for byte arrays... (but how to validate the destination (mapping) pointer is aligned for bigger accesses? hrm...)
        template< typename ZipIterator >
        static NETSER_FORCE_INLINE auto read_span(ZipIterator it)
        {
            constexpr size_t offset = ZipIterator::layout_iterator::get_offset();
            static_assert(ZipIterator::layout_iterator::get_offset() % 8 == 0, "Arrays must be aligned to byte boundaries.");

            if( Size <= UnrollMax ) {
                return detail::unroll< 0, Size, Field >::template read( it );
            } else {
                for (size_t i = 0; i < Size; ++i) {
                    read_inline< layout<Field>, mapping_list<identity_member> >(it.layout().get().template stride_offset<Field::size/8, offset/8>(i), it.mapping().dereference()[i]);
                }


                return it.advance();
            }
        }

        template< typename ZipIterator >
        static NETSER_FORCE_INLINE auto write_span(ZipIterator it)
        {
            constexpr size_t offset = ZipIterator::layout_iterator::get_offset();
            static_assert(offset % 8 == 0, "Arrays must be aligned to byte boundaries.");

            if( Size <= UnrollMax ) {
                return detail::unroll< 0, Size, Field >::template write( it );
            } else {
                for (size_t i = 0; i < Size; ++i) {
                    write_inline< layout<Field>, mapping_list<identity_member> >( it.layout().get().template stride_offset<Field::size/8, offset/8>(i), it.mapping().dereference()[i] );
                }

                return it.advance();
            }
        }

        // 
        template< typename ZipIterator, typename Generator >
        static NETSER_FORCE_INLINE void fill_random( ZipIterator it, Generator&& generator )
        {
            for( size_t i=0; i<Size; ++i) {
                ::netser::fill_mapping_random< typename layout<Field>::begin >( make_mapping_iterator< mapping_list<identity_member> >(it.dereference()[i]), std::forward<Generator>(generator) );
            }
        }

    };

}

#endif
