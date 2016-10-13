//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_WRITE_HPP__
#define NETSER_WRITE_HPP__

namespace netser {

    namespace detail {

        template< typename ZipIterator, bool IsEnd = ZipIterator::is_end >
        struct write_zip_iterator_struct {
            static NETSER_FORCE_INLINE auto write( ZipIterator it )
            {
#ifdef NETSER_DEBUG_CONSOLE
              std::cout << "Writing span:\n";
#endif
               auto result = deref_t<typename decltype(it.layout())::ct_iterator>::field::write_span( it );

#ifdef NETSER_DEBUG_CONSOLE
              std::cout << "Span written.\n";
#endif
               return write_zip_iterator_struct< decltype(result) >::write( result );
            }
        };

        template< typename ZipIterator >
        struct write_zip_iterator_struct< ZipIterator, true >
        {
            static NETSER_FORCE_INLINE auto write( ZipIterator it )
            {
                return it.layout().get().template static_offset< int(ZipIterator::layout_iterator::ct_iterator::offset) >();
            }
        };

        template< typename ZipIterator >
        NETSER_FORCE_INLINE auto write_zip_iterator( ZipIterator it )
        {
            // Need partial specialization because on each non-empty level, the iterators need to be dereferenced (which is not valid for the iterator == end)
            return write_zip_iterator_struct< ZipIterator >::write( it );
        }

    }

    template< typename Layout, typename Mapping, typename AlignedPtr, typename Arg >
    NETSER_FORCE_INLINE auto write_inline( AlignedPtr ptr, Arg&& src )
    {
        return detail::write_zip_iterator( make_zip_iterator( make_layout_iterator<Layout>( ptr ), make_mapping_iterator<Mapping>( std::forward<Arg>( src ) ) ) );
    }

    template< typename Layout, typename Mapping, typename AlignedPtr, typename Arg >
    void write( AlignedPtr ptr, Arg&& src )
    {
        detail::write_zip_iterator( make_zip_iterator( make_layout_iterator<Layout>( ptr ), make_mapping_iterator<Mapping>( std::forward<Arg>( src ) ) ) );
    }

}

#endif
