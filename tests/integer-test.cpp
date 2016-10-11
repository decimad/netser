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

    EXPECT_TRUE( log.size() == 1 && log[0].offset == 0 && log[0].size == 4 );
    log.clear();

    // Two short reads
    read<uint_layout, uint_mapping>( make_aligned_ptr<2, 0>( &src, &log ), dest );
    EXPECT_TRUE(
        log.size() == 2 &&
        log[0].offset == 0 && log[0].size == 2 &&
        log[1].offset == 2 && log[1].size == 2
    );
    log.clear();

    // Four byte reads
    read<uint_layout, uint_mapping>( make_aligned_ptr<1, 0>( &src, &log ), dest );
    EXPECT_TRUE( log.size() == 4 );
    EXPECT_TRUE( log[0].offset == 0 && log[0].size == 1 );
    EXPECT_TRUE( log[1].offset == 1 && log[1].size == 1 );
    EXPECT_TRUE( log[2].offset == 2 && log[2].size == 1 );
    EXPECT_TRUE( log[3].offset == 3 && log[3].size == 1 );
    log.clear();
}

GTEST_TEST(integer_test, single_write32)
{
    uint32_t src               = 0xd34db33f;
    const uint32_t src_swapped = 0x3fb34dd3;
    uint32_t dest = 0;
    collect_logger log;

    using uint_layout = layout< net_uint32 >;
    using uint_mapping = mapping_list< identity >;

    // Single writes
    write<uint_layout, uint_mapping>( make_aligned_ptr<4, 0>( &dest, &log ), src );

    EXPECT_EQ( dest, src_swapped );
    ASSERT_EQ( log.size(), 1 );
    EXPECT_TRUE( log[0].offset == 0 && log[0].size == 4 );
    dest = 0;
    log.clear();

    // Two short writes
    write<uint_layout, uint_mapping>( make_aligned_ptr<2, 0>( &dest, &log ), src );

    EXPECT_EQ( dest, src_swapped );
    ASSERT_EQ( log.size(), 2 );
    EXPECT_TRUE( log[0].offset == 0 && log[0].size == 2 );
    EXPECT_TRUE( log[1].offset == 2 && log[1].size == 2 );
    log.clear();
    dest = 0;

    // Four byte writes
    write<uint_layout, uint_mapping>( make_aligned_ptr<1, 0>( &dest, &log ), src );
    EXPECT_EQ( dest, src_swapped );
    ASSERT_EQ( log.size(), 4 );
    EXPECT_TRUE( log[0].offset == 0 && log[0].size == 1 );
    EXPECT_TRUE( log[1].offset == 1 && log[1].size == 1 );
    EXPECT_TRUE( log[2].offset == 2 && log[2].size == 1 );
    EXPECT_TRUE( log[3].offset == 3 && log[3].size == 1 );
    log.clear();
    dest = 0;
}

GTEST_TEST( integer_test, single_write48 )
{
    uint64_t       src         = 0xb33fd34db33full;
    const uint64_t src_swapped = 0x3fb34dd33fb3ull;
    uint64_t dest = 0;
    collect_logger log;

    using uint_layout = layout< net_uint<48> >;
    using uint_mapping = mapping_list< identity >;

    // Alignment == 4 && Defect == 0 -> Read dword, then word
    write<uint_layout, uint_mapping>( make_aligned_ptr<4, 0>( &dest, &log ), src );
    EXPECT_EQ( dest, src_swapped );
    ASSERT_EQ( log.size(), 2 );
    EXPECT_EQ( log[0].offset, 0 ); EXPECT_EQ( log[0].size, 4 );
    EXPECT_EQ( log[1].offset, 4 ); EXPECT_EQ( log[1].size, 2 );
    dest = 0;
    log.clear();

    // Alignment == 4 && Defect == 1 -> byte, word, word, byte
    write<uint_layout, uint_mapping>( make_aligned_ptr<4, 1>( &dest, &log ), src );
    EXPECT_EQ( dest, src_swapped );
    ASSERT_EQ( log.size(), 4 );
    EXPECT_EQ( log[0].offset, 0 ); EXPECT_EQ( log[0].size, 1 );
    EXPECT_EQ( log[1].offset, 1 ); EXPECT_EQ( log[1].size, 2 );
    EXPECT_EQ( log[2].offset, 3 ); EXPECT_EQ( log[2].size, 2 );
    EXPECT_EQ( log[3].offset, 5 ); EXPECT_EQ( log[3].size, 1 );
    dest = 0;
    log.clear();

    // Alignment == 4 && Defect == 2 -> Read word then dword
    write<uint_layout, uint_mapping>( make_aligned_ptr<4, 2>( &dest, &log ), src );
    EXPECT_EQ( dest, src_swapped );
    ASSERT_EQ( log.size(), 2 );
    EXPECT_EQ( log[0].offset, 0 ); EXPECT_EQ( log[0].size, 2 );
    EXPECT_EQ( log[1].offset, 2 ); EXPECT_EQ( log[1].size, 4 );
    dest = 0;
    log.clear();

    // Alignment == 4 && Defect == 3 -> byte, dword, byte
    write<uint_layout, uint_mapping>( make_aligned_ptr<4, 3>( &dest, &log ), src );
    EXPECT_EQ( dest, src_swapped );
    ASSERT_EQ( log.size(), 3 );
    EXPECT_EQ( log[0].offset, 0 ); EXPECT_EQ( log[0].size, 1 );
    EXPECT_EQ( log[1].offset, 1 ); EXPECT_EQ( log[1].size, 4 );
    EXPECT_EQ( log[2].offset, 5 ); EXPECT_EQ( log[2].size, 1 );
    dest = 0;
    log.clear();
}
