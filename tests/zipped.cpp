#include "test_shared.hpp"
#include <array>
#include <gtest/gtest.h>
#include <netser/fill_random.hpp>


using namespace netser;

// Using a PTP header packet as testcase.

using uint16 = unsigned short;
using uint8 = unsigned char;
using int8 = signed char;

struct ClockIdentity
{
    std::array<uint8, 8> identity;

    bool operator==(const ClockIdentity &other) const
    {
        return identity == other.identity;
    }
};

// ClockIdentity
using clock_identity_zipped = netser::zipped<net_uint<8>[8], MEMBER1(ClockIdentity::identity)>;

clock_identity_zipped default_zipped(ClockIdentity);

struct PortIdentity
{
    ClockIdentity clock;
    uint16 port;

    bool operator==(const PortIdentity &other) const
    {
        return clock == other.clock && port == other.port;
    }
};

// PortIdentity
using port_identity_zipped = netser::zipped<ZIPPED_MEMBER(PortIdentity::clock), net_uint<16>, MEMBER1(PortIdentity::port)>;

port_identity_zipped default_zipped(PortIdentity);

struct Header
{
    enum class Field0Flags
    {
        AlternateMaster = 0x01,
        TwoStep = 0x02,
        Unicast = 0x04,
        ProfileSpecific1 = 0x20,
        ProfileSpecific2 = 0x40,
        Security = 0x80,
    };

    enum class Field1Flags
    {
        Li61 = 1,
        Li59 = 2,
        UtcValid = 4,
        PtpTimescale = 8,
        TimeTraceable = 16,
        FrequencyTraceable = 32
    };

    PortIdentity source_port_identity;
    int64 correction_field;
    uint16 message_length;
    uint16 sequence_id;
    uint8 transport_specific;
    uint8 message_type;
    uint8 version_ptp;
    uint8 domain_number;
    uint8 flag_field0;
    uint8 flag_field1;
    uint8 control_field;
    int8 log_message_interval;

    bool operator==(const Header &other) const
    {
        return source_port_identity == other.source_port_identity && correction_field == other.correction_field
               && message_length == other.message_length && sequence_id == other.sequence_id
               && transport_specific == other.transport_specific && message_type == other.message_type && version_ptp == other.version_ptp
               && domain_number == other.domain_number && flag_field0 == other.flag_field0 && flag_field1 == other.flag_field1
               && control_field == other.control_field && log_message_interval == other.log_message_interval;
    }
};

using header_zipped = netser::zipped<net_uint<4>, MEMBER1(Header::transport_specific), net_uint<4>, MEMBER1(Header::message_type),
                                     reserved<4>, net_uint<4>, MEMBER1(Header::version_ptp), net_uint<16>, MEMBER1(Header::message_length),
                                     net_uint<8>, MEMBER1(Header::domain_number), reserved<8>, net_uint<8>, MEMBER1(Header::flag_field0),
                                     net_uint<8>, MEMBER1(Header::flag_field1), net_uint<64>, MEMBER1(Header::correction_field),
                                     reserved<32>, ZIPPED_MEMBER(Header::source_port_identity), net_uint<16>, MEMBER1(Header::sequence_id),
                                     net_uint<8>, MEMBER1(Header::control_field), net_int<8>, MEMBER1(Header::log_message_interval)>;

header_zipped default_zipped(Header);

// Test PTP header roundtrips.
GTEST_TEST(zipped, zipped_ptp_header_roundtrip)
{
    char buffer[128];
    Header header_src;
    Header header_dest;

    fill_random(header_src);
    make_aligned_ptr<1>(buffer) << header_src;
    make_aligned_ptr<1>(buffer) >> header_dest;
    EXPECT_EQ(header_src, header_dest);

    fill_random(header_src);
    make_aligned_ptr<2>(buffer) << header_src;
    make_aligned_ptr<2>(buffer) >> header_dest;
    EXPECT_EQ(header_src, header_dest);

    fill_random(header_src);
    make_aligned_ptr<4, 0, 34>(buffer) << header_src;
    make_aligned_ptr<4, 0, 34>(buffer) >> header_dest;
    EXPECT_EQ(header_src, header_dest);

    fill_random(header_src);
    make_aligned_ptr<4, 1, 34>(buffer) << header_src;
    make_aligned_ptr<4, 1, 34>(buffer) >> header_dest;
    EXPECT_EQ(header_src, header_dest);

    fill_random(header_src);
    make_aligned_ptr<4, 2, 34>(buffer) << header_src;
    make_aligned_ptr<4, 2, 34>(buffer) >> header_dest;
    EXPECT_EQ(header_src, header_dest);

    fill_random(header_src);
    make_aligned_ptr<4, 3>(buffer) << header_src;
    make_aligned_ptr<4, 3>(buffer) >> header_dest;
    EXPECT_EQ(header_src, header_dest);
}
