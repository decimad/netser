//          Copyright Michael Steinberg 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef NETSER_INTEGER_HPP__
#define NETSER_INTEGER_HPP__

#ifdef NETSER_DEBUG_CONSOLE
#include <iostream>
#endif
#include <netser/layout.hpp>
#include <netser/mem_access.hpp>

namespace netser {

    template< bool Signed, size_t Bits, typename Endianess >
    struct int_;

    namespace detail {

        template< typename T >
        struct decode_integer {
            static constexpr bool value = false;
        };

        template< bool Signed, size_t Bits, typename Endianess >
        struct decode_integer< int_< Signed, Bits, Endianess > >
        {
            static constexpr bool is_signed = Signed;
            static constexpr size_t bits = Bits;
            using endianess = Endianess;

            static constexpr bool value = true;
        };

        template< size_t Bits >
        using auto_stage_type_t = std::conditional_t< (Bits <= 8), unsigned char,
            std::conditional_t< (Bits <= 16), unsigned short,
            std::conditional_t< (Bits <= 32), unsigned int,
            std::conditional_t< (Bits <= 64), unsigned long long, error_type > > > >;

    }

    //===========================
    // read_span helper types
    //
    namespace detail {

        //
        // partial_field_access
        //
        template< typename PlacedAccess, typename PlacedField >
        struct partial_field_access {
            using placed_access = PlacedAccess;
            using placed_field  = PlacedField;

            using field_range          = typename placed_field::range;
            using access_integral_type = typename placed_access::type;

            using access_range         = typename placed_access::range;
            using intersection_range   = intersection< typename placed_access::range, field_range >;

            using access_endianess = typename placed_access::endianess;
            using field_endianess = typename placed_field::field::endianess;

            static constexpr size_t post_read_shift_down = (field_range::end < access_range::end) ? (access_range::end - field_range::end) : 0;
            static constexpr size_t pre_assemble_shift_up = (access_range::end < field_range::end) ? (field_range::end - access_range::end) : 0;

            template< typename StageType, typename AlignedPtr >
            static StageType read(AlignedPtr ptr)
            {
                return static_cast<StageType>((conditional_swap<!std::is_same<access_endianess, field_endianess>::value>(placed_access::read(ptr)) >> post_read_shift_down) & bit_mask<access_integral_type>(intersection_range::size)) << pre_assemble_shift_up;
            }
        };

        //
        // better_access
        //

        namespace impl {

            template< typename PartialAccessA, typename PartialAccessB >
            struct better_access {
                using type = std::conditional_t<
                    (PartialAccessA::intersection_range::size > PartialAccessB::intersection_range::size),
                    PartialAccessA,
                    PartialAccessB
                >;
            };

        } // impl


        namespace impl {
            template< typename PartialAccessA, typename PartialAccessB >
            using better_access_t = typename impl::better_access<PartialAccessA, PartialAccessB>::type;

            //
            // find_best_access
            //
            template<
                typename LayoutIterator,
                int Offset,
                typename PossibleAccesses,
                typename CurrentBest = empty_access
            >
            struct find_best_access;

            template<
                typename LayoutIterator,
                int Offset,
                typename CurrentBest
            >
            struct find_best_access< LayoutIterator, Offset, type_list< >, CurrentBest >
            {
                static_assert(!std::is_same<CurrentBest, empty_access>::value, "No matching read!");
                using type = CurrentBest;
                static constexpr size_t next_offset = type::access_range::end;

#ifdef NETSER_DEBUG_CONSOLE
                static void describe() {
                    std::cout << "    ....Done. Best access size: " << type::placed_access::access::size << "(" << CurrentBest::access_range::begin << "..." << CurrentBest::access_range::end << ")\n";
                }
#endif
            };

            template<
                typename LayoutIterator,
                int Offset,
                typename Access0, typename... AccessList,
                typename CurrentBest
            >
            struct find_best_access< LayoutIterator, Offset, type_list< Access0, AccessList... >, CurrentBest >
            {
                using call_type = find_best_access<
                    LayoutIterator,
                    Offset,
                    type_list< AccessList... >,
                    better_access_t< partial_field_access< placed_memory_access< Access0, Offset, typename LayoutIterator::pointer_type >, deref_t< LayoutIterator > >, CurrentBest >
                >;

                using type = typename call_type::type;

#ifdef NETSER_DEBUG_CONSOLE
                static void describe() {
                    std::cout << "    At offset " << Offset << " checking access with size " << Access0::size << ".\n";
                    call_type::describe();
                }
#endif
            };
        }

        template< typename LayoutIterator, int Offset, typename PossibleAccessesList = platform_filtered_memory_accesses< LayoutIterator::get_max_alignment() /*Alignment*/>, typename CurrentResult = empty_access >
        using find_best_access_t = typename impl::find_best_access< LayoutIterator, Offset, PossibleAccessesList, CurrentResult >::type;

#ifdef NETSER_DEBUG_CONSOLE
        template< typename LayoutIterator, int Offset, typename PossibleAccessesList = platform_filtered_memory_accesses<LayoutIterator::get_max_alignment() /*Alignment*/>, typename CurrentResult = empty_access >
        void describe_find_best_access_t() {
            std::cout << "\n    Finding best access at offset " << Offset <<  " out of " << list_size_v< PossibleAccessesList > << " alternatives.\n";
            impl::find_best_access< LayoutIterator, Offset, PossibleAccessesList, CurrentResult >::describe();
        }
#endif

        namespace impl {

            template< typename LayoutIterator, typename StageType, int Offset, typename PartialFieldAccessList = type_list< >, bool IsFinished = (Offset >= deref_t<LayoutIterator>::range::end) >
            struct generate_memory_access_list_struct {
                using type = PartialFieldAccessList;

#ifdef NETSER_DEBUG_CONSOLE
                static void describe()
                {
                    std::cout << "Complete!\n";

                }
#endif
            };

            template< typename LayoutIterator, typename StageType, int Offset, typename... PartialFieldAccess >
            struct generate_memory_access_list_struct< LayoutIterator, StageType, Offset, type_list< PartialFieldAccess... >, false >
            {
                static constexpr size_t alignment   = LayoutIterator::get_access_alignment(Offset);
                using placed_field                  = deref_t< LayoutIterator >;

                using access    = find_best_access_t< LayoutIterator, Offset >;
                using list      = type_list< PartialFieldAccess..., access >;
                using call_type = generate_memory_access_list_struct< LayoutIterator, StageType, access::access_range::end, list >;
                using type      = typename call_type::type;

#ifdef NETSER_DEBUG_CONSOLE
                static void describe()
                {
                    describe_find_best_access_t< LayoutIterator, Offset>();
                    std::cout << "    Access (" << access::access_range::begin << "..." << access::access_range::end << ") \n";
                    call_type::describe();
                }
#endif
            };

        }

        template< typename LayoutIterator, typename StageType >
        using generate_partial_memory_access_list_t = typename impl::generate_memory_access_list_struct< LayoutIterator, StageType, LayoutIterator::get_offset() >::type;

#ifdef NETSER_DEBUG_CONSOLE
        template< typename LayoutIterator, typename StageType >
        void describe_generate_partial_memory_access_list_t() {
            impl::generate_memory_access_list_struct< LayoutIterator, StageType, LayoutIterator::get_offset() >::describe();
        }
#endif

        // ===============
        // run_access_list
        // runs and assembles all partial_field_access'es in a list
        template< typename PartialFieldAccessList >
        struct run_access_list
        {
            template< typename StageType, typename AlignedPtr >
            static NETSER_FORCE_INLINE StageType run(AlignedPtr) {
                return StageType(0);
            }
#ifdef NETSER_DEBUG_CONSOLE
            template< typename StageType, typename AlignedPtr >
            static NETSER_FORCE_INLINE StageType run_describe( AlignedPtr ) {
                return StageType( 0 );
            }
#endif
        };

        template< typename Access0, typename... Tail >
        struct run_access_list< type_list< Access0, Tail... > >
        {
            template< typename StageType, typename AlignedPtr >
            static NETSER_FORCE_INLINE StageType run(AlignedPtr ptr)
            {
#ifdef NETSER_DEBUG_CONSOLE
                std::cout << "Access of size " << Access0::placed_access::access::size << "\n";
#endif
                return Access0::template read<StageType>(ptr) | run_access_list< type_list< Tail... > >::template run<StageType>(ptr);
            }

        };


    } // detail

    //=============
    // write_span helper types
    //

    namespace detail {

        template< typename Type >
        struct is_integer_struct : public std::false_type {};

        template< bool Signed, size_t Bits, typename ByteOrder >
        struct is_integer_struct< int_< Signed, Bits, ByteOrder > > : public std::true_type {};

    }

    // true iff T is an instance of netser::int_
    template< typename T >
    constexpr bool is_integer_v = detail::is_integer_struct< T >::value;

    namespace detail3 {

        enum class discover_case {
            add_field,
            grow_write,
            finish
        };

        template< typename CtLayoutIterator, size_t FieldWrittenBits, typename AccessList, size_t CollectedBits = 0 >
        struct discover_access;

        template< typename CtLayoutIterator, size_t FieldWrittenBits, typename AccessList, size_t CollectedBits, discover_case Case >
        struct discover_switch;

        template< typename CtLayoutIterator, size_t FieldWrittenBits, typename AccessList, size_t CollectedBits >
        struct discover_switch< CtLayoutIterator, FieldWrittenBits, AccessList, CollectedBits, discover_case::add_field >
        {
            using type = typename discover_access<
                next_t<CtLayoutIterator>,
                0,
                AccessList,
                CollectedBits + deref_t<CtLayoutIterator>::size - FieldWrittenBits
            >::type;
        };

        template< typename CtLayoutIterator, size_t FieldWrittenBits, typename AccessList, size_t CollectedBits >
        struct discover_switch< CtLayoutIterator, FieldWrittenBits, AccessList, CollectedBits, discover_case::grow_write >
        {
            using type = typename discover_access<
                CtLayoutIterator,
                FieldWrittenBits,
                pop_front_t<AccessList>,
                CollectedBits
            >::type;
        };

        template< typename CtLayoutIterator, size_t FieldWrittenBits, typename AccessList, size_t CollectedBits >
        struct discover_switch< CtLayoutIterator, FieldWrittenBits, AccessList, CollectedBits, discover_case::finish >
        {
            using type = front_t< AccessList >;
        };

        // WrittenBits = bits of the Layout front that have already been written
        // AccessList  = list of access, sorted in ascending size (This list must have been cleared of accesses that are unaligned wrt. the layout-iterator)
        // WriteBits   = The amount of bits that have been assembled for this write so far
        template< typename CtLayoutIterator, size_t FieldWrittenBits, typename AccessList, size_t CollectedBits >
        struct discover_access {
            static_assert( !CtLayoutIterator::is_end, "Could not get enough fields" );
            static_assert( !is_integer_v< CtLayoutIterator >, "No integer field" );

            using write = front_t< AccessList >;

            using field = deref_t<CtLayoutIterator>;    // => placed_field
            static constexpr size_t field_remaining_bits = field::size - FieldWrittenBits;

            static constexpr bool is_finished     = field_remaining_bits + CollectedBits == write::size*8;
            static constexpr bool need_more_data  = field_remaining_bits + CollectedBits < write::size*8;
            static constexpr bool need_more_write = field_remaining_bits + CollectedBits > write::size*8;
            static constexpr bool can_grow_write  = (list_size<AccessList>::value) > 1;

            static constexpr discover_case this_case =
                (is_finished || (need_more_write && !can_grow_write)) ? discover_case::finish :
                    ( need_more_data ? discover_case::add_field : discover_case::grow_write );

            using type = typename discover_switch<
                CtLayoutIterator,
                FieldWrittenBits,
                AccessList,
                CollectedBits,
                this_case
            >::type;
        };

    }


    namespace detail2 {

        struct write_integer_algorithm {
        private:
            enum class execute_action {
                write,
                write_and_continue,
                collect_all,
                collect_partial,
                error
            };

            static constexpr execute_action determine_execute_action(size_t access_size, size_t access_written, size_t field_size, size_t field_written)
            {
                return
                    (access_written > access_size) ?
                        execute_action::error
                        :
                        ((access_written == access_size) ?
                            ((field_written == 0) ?
                                execute_action::write
                                :
                                execute_action::write_and_continue)
                            :
                            ((access_written + field_size - field_written <= access_size) ?
                                execute_action::collect_all
                                :
                                execute_action::collect_partial)
                        );
            }

            template<typename PlacedAccess, typename Endianess, size_t FieldSize, size_t BitsWritten = 0, size_t FieldWritten = 0, execute_action Task = determine_execute_action(PlacedAccess::size*8, BitsWritten, FieldSize, FieldWritten) >
            struct execute_access {

                template< typename T > static void execute() {}

            };

            // Access complete, no partial field remaining
            template<typename PlacedAccess, typename Endianess, size_t FieldSize, size_t BitsWritten, size_t FieldWritten>
            struct execute_access< PlacedAccess, Endianess, FieldSize, BitsWritten, FieldWritten, execute_action::write >
            {
                using type = typename PlacedAccess::type;

                template< typename ZipIterator >
                NETSER_FORCE_INLINE static auto execute(ZipIterator it, type val = 0)
                {
                    using access_endianess = typename PlacedAccess::endianess;

                    static_assert(FieldWritten == 0, "Huh?");

                    PlacedAccess::template write(
                        it.layout().get(),
                        conditional_swap< !std::is_same<access_endianess, Endianess >::value >(val)
                    );

                    return it;
                }

                template< typename ZipIterator >
                static auto describe(ZipIterator it, type val = 0)
                {
//                    static_assert(!ZipIterator::is_end, "Invalid iterator!");
                    PlacedAccess::describe();
                    return it;
                }
            };


            // Access complete, partial field remaining
            template<typename PlacedAccess, typename Endianess, size_t FieldSize, size_t BitsWritten, size_t FieldWritten>
            struct execute_access< PlacedAccess, Endianess, FieldSize, BitsWritten, FieldWritten, execute_action::write_and_continue >
            {
                using type = typename PlacedAccess::type;
                using access_endianess = typename PlacedAccess::endianess;

                template< typename ZipIterator >
                NETSER_FORCE_INLINE static auto execute(ZipIterator it, type val = 0) {
                    PlacedAccess::template write(
                        it.layout().get(),
                        conditional_swap< !std::is_same<access_endianess, Endianess >::value >(val)
                    );

#ifdef NETSER_DEBUG_CONSOLE
                    PlacedAccess::describe();
#endif
                    return write_integer<FieldWritten>(it);
                }
            };


            // Get next data (all or all remaining) -> iterator advances on this step and current data doesn't need to be shifted down.
            // Get next data (partial) -> iterator _does not_ advance on this step and current data needs to be shifted down
            // Layout field a, Memory access x
            // layout: ...aaaaaaaabbbbbbbb...        -> done
            // layout: ...aaaabbbbbbbbbbbb...        -> continue and cap on next step
            // access: ...xxxxxxxx]                  (shift down 5, mask 8 bits)
            template<typename PlacedAccess, typename Endianess, size_t FieldSize, size_t BitsWritten, size_t FieldWritten>
            struct execute_access< PlacedAccess, Endianess, FieldSize, BitsWritten, FieldWritten, execute_action::collect_all >
            {
                using type = typename PlacedAccess::type;
                static constexpr size_t num_bits = FieldSize - FieldWritten;
                static constexpr size_t shift_up = PlacedAccess::size * 8 - BitsWritten - num_bits;

                template< typename ZipIterator >
                NETSER_FORCE_INLINE static auto execute(ZipIterator it, type val = 0) {
                    return execute_access< PlacedAccess, Endianess, FieldSize, BitsWritten + num_bits, 0 >::template execute(
                        it.advance(),
                        val | ((bit_mask<type>(num_bits) & static_cast<type>(it.mapping().dereference())) << shift_up)
                    );
                }
            };

            // Get next data (partial) -> iterator _does not_ advance on this step and current data needs to be shifted down
            // Layout field a, Memory access x
            // layout: ...aaaaaaaaaaaaa...
            // access: ...xxxxxxxx]                  (shift down 5, mask 8 bits)
            template<typename PlacedAccess, typename Endianess, size_t FieldSize, size_t BitsWritten, size_t FieldWritten>
            struct execute_access< PlacedAccess, Endianess, FieldSize, BitsWritten, FieldWritten, execute_action::collect_partial >
            {
                template< size_t bits_to_write, size_t field_remaining >
                struct validate : public std::true_type {
                    static_assert(bits_to_write < field_remaining, "Huh?");
                };


                using type = typename PlacedAccess::type;
                template< typename ZipIterator >
                NETSER_FORCE_INLINE static auto execute(ZipIterator it, type val = 0) {
                    static constexpr size_t num_bits   = PlacedAccess::size*8 - BitsWritten;
                    static_assert( validate< num_bits, FieldSize - FieldWritten>::value, "Something's wrong!");
                    static constexpr size_t shift_down = FieldSize - FieldWritten - num_bits;

#ifdef NETSER_DEBUG_CONSOLE
                    std::cout << "Shifting down by " << shift_down << " bits.";
#endif

                    return execute_access< PlacedAccess, Endianess, FieldSize, BitsWritten + num_bits, FieldWritten + num_bits >::template execute(
                        it,
                        val | (bit_mask<type>(num_bits) & static_cast<type>(it.mapping().dereference() >> shift_down))
                    );
                }
            };

        public:
            template< size_t FieldWritten = 0, typename ZipIterator  >
            NETSER_FORCE_INLINE static auto write_integer( ZipIterator it )
            {
                using layout_iterator    = typename ZipIterator::layout_iterator;
                using layout_iterator_ct = typename layout_iterator::ct_iterator;
                using placed_field       = deref_t< layout_iterator_ct >;
                using field              = typename placed_field::field;

#ifdef NETSER_DEBUG_CONSOLE
                std::cout << "\n    Aligned Ptr: "; it.layout().get().describe();
                std::cout << "\n    Field offset bytes: " << layout_iterator_ct::offset / 8 << ". ";
                std::cout << "\n    Access alignment: " << layout_iterator::get_access_alignment( FieldWritten ) << " (Field written: " << FieldWritten << "), ";
#endif

                using access = typename detail3::discover_access<
                    layout_iterator_ct,
                    FieldWritten,
                    platform_filtered_memory_accesses< layout_iterator::get_access_alignment( FieldWritten ) >
                >::type;

#ifdef NETSER_DEBUG_CONSOLE
                std::cout << "\n    Would like to use size " << access::size << "\n";
#endif

                using placed_access = placed_atomic_memory_access< access, placed_field::offset + FieldWritten>;

                static_assert((placed_field::offset + FieldWritten) % 8 == 0, "Something's bad!");

                return execute_access< placed_access, typename field::endianess, field::size, 0, FieldWritten >::template execute(it);
            }

        };

    }



    // Integer default mapping (If sub-byte, it must not span byte borders, if multi-byte, it must be byte-aligned)
    template< bool Signed, size_t Bits, typename ByteOrder >
    struct int_ : public detail::simple_field_layout_mixin< int_<Signed, Bits, ByteOrder> > {

        //
        // Basic Layout interface
        //
        static constexpr size_t count = 1;
        static constexpr size_t size = Bits;

        using endianess = ByteOrder;

        template< size_t Index, size_t BitOffset >
        struct get_field {
            static_assert(Index == 0, "Error!");
            static constexpr size_t offset = BitOffset;
            using type = int_;
        };

        //
        // Basic leaf interface
        //
        using stage_type    = detail::auto_stage_type_t< Bits >;
        using integral_type = std::conditional_t< Signed, std::make_signed_t< stage_type >, std::make_unsigned_t< stage_type > >;

        template< typename LayoutIterator >
        static constexpr NETSER_FORCE_INLINE integral_type read(LayoutIterator layit)
        {
#ifdef NETSER_DEBUG_CONSOLE
            std::cout << "Generating memory access List (";
            std::cout << "Field size: " << deref_t<typename LayoutIterator::ct_iterator>::size << " bits. Ptr-Alignment: " << layit.get_access_alignment( 0 ) << ")... ";
#endif
            using accesses = detail::generate_partial_memory_access_list_t<LayoutIterator, unsigned int >;

#ifdef NETSER_DEBUG_CONSOLE
            detail::describe_generate_partial_memory_access_list_t<LayoutIterator, unsigned int>();
            std::cout << "Got " << list_size_v<accesses> << " reads.\n";
#endif
            return static_cast<integral_type>(detail::template run_access_list< accesses >::template run< stage_type >(layit.get()));
        }

        template< typename ZipIterator >
        static NETSER_FORCE_INLINE auto read_span( ZipIterator it )
        {
            static_assert(!std::is_const<decltype(it.mapping())>::value, "Error!");
            static_assert(!std::is_const<decltype(it.mapping().dereference())>::value, "Error!");

#ifdef NETSER_DEBUG_CONSOLE
            std::cout << "reading integer span:\n";
#endif
            it.mapping().dereference() = read(it.layout());

#ifdef NETSER_DEBUG_CONSOLE
            std::cout << "read span complete.\n";
#endif
            return it.advance();
        }

        template< typename ZipIterator >
        static NETSER_FORCE_INLINE auto write_span( ZipIterator it )
        {
            // The new implementation takes two steps to calculate the next write(s).
            // 1. Look ahead the iterator sequence to discover what access type to use
            // 2. Complete this write
            // 3. If any field was not committed completely, restart at step 1.

            // This happens in order to decrease the lookahead depth and the amount of data which is taken along the iterations.
            // Involved type names get very longish very quickly, so this implementation tries to keep them as short as possible.

            return detail2::write_integer_algorithm::write_integer<>(it);
        }

        template< typename DestType, typename T >
        static constexpr DestType extract(T val) {
            return static_cast<DestType>(val) & bit_mask<DestType>(size);
        }

    };

    template< size_t Size >
    using net_uint = int_< false, Size, be >;

    template< size_t Size >
    using net_int = int_< true, Size, be >;

    using net_int8   = net_int<8>;
    using net_int16  = net_int<16>;
    using net_int32  = net_int<32>;

    using net_uint8  = net_uint<8>;
    using net_uint16 = net_uint<16>;
    using net_uint32 = net_uint<32>;

}

#endif
