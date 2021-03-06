//#define NETSER_DEBUG_CONSOLE
//#define NETSER_DEREFERENCE_LOGGING
#include <netser/netser.hpp>
#include <netser/range.hpp>
#include <merge_sort.hpp>
#include <string>
#include <cassert>
#include <netser/fill_random.hpp>
#include <netser/static_array.hpp>
#include <iostream>

using netser::type_list;
using netser::next_t;
using netser::deref_t;
/*
class print_logger : public netser::dereference_logger
{
public:
    void log( uintptr_t memory_location, int offset, std::string type_name, size_t type_size, size_t type_alignment ) override
    {
        std::cout << "Access at " << std::hex << memory_location << " (+" << offset << "): " << type_name << " (size: " << type_size << ", align: " << type_alignment << ")\n";
    }
};
*/
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

using namespace netser;
#include <array>

// Using a PTP header packet as testcase.

using int32 = int;
using int16 = short;
using uint16 = unsigned short;
using uint8 = unsigned char;
using int8 = signed char;

struct ClockIdentity {
    std::array<uint8, 8> identity;

    bool operator==( const ClockIdentity& other ) const
    {
        return identity == other.identity;
    }
};

// ClockIdentity
using clock_identity_zipped = netser::zipped<
    net_uint<8>[8], MEMBER1( ClockIdentity::identity )
>;

clock_identity_zipped default_zipped( ClockIdentity );

struct PortIdentity {
    ClockIdentity clock;
    uint16 port;

    bool operator==( const PortIdentity& other ) const
    {
        return clock == other.clock
            && port == other.port;
    }
};

// PortIdentity
using port_identity_zipped = netser::zipped<
    ZIPPED_MEMBER( PortIdentity::clock )
    , net_uint<16>, MEMBER1( PortIdentity::port )
>;

port_identity_zipped default_zipped( PortIdentity );

struct Header {
    enum class Field0Flags {
        AlternateMaster = 0x01,
        TwoStep = 0x02,
        Unicast = 0x04,
        ProfileSpecific1 = 0x20,
        ProfileSpecific2 = 0x40,
        Security = 0x80,
    };

    enum class Field1Flags {
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

    bool operator==( const Header& other ) const
    {
        return source_port_identity == other.source_port_identity
            && correction_field == other.correction_field
            && message_length == other.message_length
            && sequence_id == other.sequence_id
            && transport_specific == other.transport_specific
            && message_type == other.message_type
            && version_ptp == other.version_ptp
            && domain_number == other.domain_number
            && flag_field0 == other.flag_field0
            && flag_field1 == other.flag_field1
            && control_field == other.control_field
            && log_message_interval == other.log_message_interval;
    }
};

using header_zipped = netser::zipped<
    net_uint<4>, MEMBER1( Header::transport_specific )
    , net_uint<4>, MEMBER1( Header::message_type )
    , reserved<4>
    , net_uint<4>, MEMBER1( Header::version_ptp )
    , net_uint<16>, MEMBER1( Header::message_length )
    , net_uint<8>, MEMBER1( Header::domain_number )
    , reserved<8>
    , net_uint<8>, MEMBER1( Header::flag_field0 )
    , net_uint<8>, MEMBER1( Header::flag_field1 )
    , net_uint<64>, MEMBER1( Header::correction_field )
    , reserved<32>
    , ZIPPED_MEMBER( Header::source_port_identity )
    , net_uint<16>, MEMBER1( Header::sequence_id )
    , net_uint<8>, MEMBER1( Header::control_field )
    , net_int<8>, MEMBER1( Header::log_message_interval )
>;

header_zipped default_zipped( Header );


struct Time {
    int64 secs_;
    int32 nanos_;
};

// Time
using time_zipped = netser::zipped <
    net_uint<48>, MEMBER1( Time::secs_ )
    , net_uint<32>, MEMBER1( Time::nanos_ )
>;

time_zipped default_zipped( Time );

struct ClockQuality {
    uint16 offset_scaled_log_variance;
    uint8 clock_class;
    uint8 clock_accuracy;
};

// ClockQuality
using clock_quality_zipped = netser::zipped<
    net_uint<8>, MEMBER1( ClockQuality::clock_class )
    , net_uint<8>, MEMBER1( ClockQuality::clock_accuracy )
    , net_uint<16>, MEMBER1( ClockQuality::offset_scaled_log_variance )
>;

clock_quality_zipped default_zipped( ClockQuality );

struct Announce {
    Time origin_timestamp;
    ClockQuality grandmaster_clock_quality;
    ClockIdentity grandmaster_identity;
    int16 current_utc_offset;
    uint16 steps_removed;

    uint8 grandmaster_priority1;
    uint8 grandmaster_priority2;

    enum class TimeSource : uint8 {
        AtomicClock = 0x10,
        GPS = 0x20,
        TerrestrialRadio = 0x30,
        PTP = 0x40,
        NTP = 0x50,
        HandSet = 0x60,
        Other = 0x90,
        InternalOscillator = 0xA0
    };

    TimeSource time_source;
};

// Announce
using announce_zipped = netser::zipped<
    ZIPPED_MEMBER( Announce::origin_timestamp )
    , net_uint<16>, MEMBER1( Announce::current_utc_offset )
    , net_uint<8>, netser::constant<unsigned char, 0>
    , net_uint<8>, MEMBER1( Announce::grandmaster_priority1 )
    , ZIPPED_MEMBER( Announce::grandmaster_clock_quality )
    , net_uint<8>, MEMBER1( Announce::grandmaster_priority2 )
    , ZIPPED_MEMBER( Announce::grandmaster_identity )
    , net_uint<16>, MEMBER1( Announce::steps_removed )
    , net_uint<8>, MEMBER1( Announce::time_source )
>;

announce_zipped default_zipped( Announce );

int main()
{
    char buffer[128];
    Header header_src;
    Header header_dest;

    static_assert(-3 % 4 % 4 == -3, "foo!");

    fill_random( header_src );
    make_aligned_ptr<1>( buffer ) << header_src;
    make_aligned_ptr<1>( buffer ) >> header_dest;

    fill_random( header_src );
    make_aligned_ptr<2>( buffer ) << header_src;
    make_aligned_ptr<2>( buffer ) >> header_dest;

    fill_random( header_src );
    make_aligned_ptr<4, 0, 34>( buffer ) << header_src;
    make_aligned_ptr<4, 0, 34>( buffer ) >> header_dest;

    fill_random( header_src );
    make_aligned_ptr<4, 1, 34>( buffer ) << header_src;
    make_aligned_ptr<4, 1, 34>( buffer ) >> header_dest;

    fill_random( header_src );
    make_aligned_ptr<4, 2, 34>( buffer ) << header_src;
    make_aligned_ptr<4, 2, 34>( buffer ) >> header_dest;

    fill_random( header_src );
    make_aligned_ptr<4, 3>( buffer ) << header_src;
    make_aligned_ptr<4, 3>( buffer ) >> header_dest;

    Announce announce;
    fill_random( announce );
    make_aligned_ptr<4, 0, 34>( buffer ) << announce;
}
