#include <gtest/gtest.h>
#include "test_shared.hpp"

using namespace netser;

GTEST_TEST(integer_test, single_read32)
{
    uint32_t src;
    uint32_t dest;
    collect_logger log;

    using uint_layout  = layout< net_uint32 >;
    using uint_mapping = mapping_list< identity >;

    // Single read
    read<uint_layout, uint_mapping>( make_aligned_ptr<4, 0>(&src, &log), dest );

    EXPECT_EQ( log.size(), 1 );
    if( log.size() == 1 ) {
        EXPECT_EQ(log[0].offset, 0); EXPECT_EQ(log[0].size, 4 );
    }
    log.clear();

    // Two short reads
    read<uint_layout, uint_mapping>( make_aligned_ptr<2, 0>( &src, &log ), dest );
    EXPECT_EQ( log.size(), 2 );
    if( log.size() == 2 ) {
        EXPECT_EQ(log[0].offset, 0); EXPECT_EQ(log[0].size, 2);
        EXPECT_EQ(log[1].offset, 2); EXPECT_EQ(log[1].size, 2);
    }

    log.clear();

    // Four byte reads
    read<uint_layout, uint_mapping>( make_aligned_ptr<1, 0>( &src, &log ), dest );
    EXPECT_EQ( log.size(), 4 );
    if( log.size() == 4 ) {
        EXPECT_EQ(log[0].offset, 0); EXPECT_EQ(log[0].size, 1);
        EXPECT_EQ(log[1].offset, 1); EXPECT_EQ(log[1].size, 1);
        EXPECT_EQ(log[2].offset, 2); EXPECT_EQ(log[2].size, 1);
        EXPECT_EQ(log[3].offset, 3); EXPECT_EQ(log[3].size, 1);
    }
    log.clear();
}

GTEST_TEST( integer_test, single_read16_defect_range )
{
    unsigned short src[] = { 0xffff, 0x1234 };
    unsigned short src_swapped = 0x3412;
    uint32_t dest;
    collect_logger log;

    using uint_layout = layout< net_uint16 >;
    using uint_mapping = mapping_list< identity >;

    // Single read, aligned
    read<uint_layout, uint_mapping>(make_aligned_ptr<2, 0, 0, netser::two_side_limit_range<0, 3>>(&src[1], &log), dest);
    EXPECT_EQ(dest, src_swapped);
    EXPECT_EQ(log.size(), 1);
    if( log.size() == 1 ) {
        EXPECT_EQ(log[0].offset, 0); EXPECT_EQ(log[0].size, 2);
    }
    log.clear();

    // Single read, unaligned, no backspace, to tailspace
    read<uint_layout, uint_mapping>(make_aligned_ptr<4, 1, 0, netser::two_side_limit_range<0, 3>>(&src[1], &log), dest);
    EXPECT_EQ(dest, src_swapped);
    EXPECT_EQ(log.size(), 2);
    if( log.size() == 2 ) {
        EXPECT_EQ(log[0].offset, 0 ); EXPECT_EQ( log[0].size, 1);
        EXPECT_EQ(log[1].offset, 1 ); EXPECT_EQ( log[1].size, 1);
    }
    log.clear();

    // Single read, unaligned, backspace, no tailspace
    read<uint_layout, uint_mapping>(make_aligned_ptr<4, 1, 0, netser::two_side_limit_range<-2, 5>>(&src[1], &log), dest);
    EXPECT_EQ(dest, src_swapped);
    EXPECT_EQ(log.size(), 1);
    if( log.size() == 2 ) {
        EXPECT_EQ(log[0].offset, -1); EXPECT_EQ(log[0].size, 4);
        log.clear();
    }
}

