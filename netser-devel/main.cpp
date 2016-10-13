#define NETSER_DEBUG_CONSOLE
#define NETSER_DEREFERENCE_LOGGING
#include <netser/netser.hpp>
#include <netser/range.hpp>
#include <merge_sort.hpp>
#include <string>
#include <cassert>
#include <netser/fill_random.hpp>
#include <netser/static_array.hpp>

using netser::type_list;
using netser::next_t;
using netser::deref_t;

class print_logger : public netser::dereference_logger
{
public:
    void log( uintptr_t memory_location, int offset, std::string type_name, size_t type_size, size_t type_alignment ) override
    {
        std::cout << "Access at " << std::hex << memory_location << " (+" << offset << "): " << type_name << " (size: " << type_size << ", align: " << type_alignment << ")\n";
    }
};

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

#include <array>
using namespace netser;

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

struct PortIdentity {
    ClockIdentity clock;
    uint16 port;

    bool operator==( const PortIdentity& other ) const
    {
        return clock == other.clock
            && port  == other.port;
    }
};

// PortIdentity
using port_identity_zipped = netser::zipped<
    ZIPPED_MEMBER( clock_identity_zipped, PortIdentity::clock )
    , net_uint<16>, MEMBER1( PortIdentity::port )
>;

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
      net_uint< 4>, MEMBER1( Header::transport_specific )
    , net_uint< 4>, MEMBER1( Header::message_type )
    , reserved< 4>
    , net_uint< 4>, MEMBER1( Header::version_ptp )
    , net_uint<16>, MEMBER1( Header::message_length )
    , net_uint< 8>, MEMBER1( Header::domain_number )
    , reserved< 8>
    , net_uint< 8>, MEMBER1( Header::flag_field0 )
    , net_uint< 8>, MEMBER1( Header::flag_field1 )
    , net_uint<64>, MEMBER1( Header::correction_field )
    , reserved<32>
    , ZIPPED_MEMBER( port_identity_zipped, Header::source_port_identity )
    , net_uint<16>, MEMBER1( Header::sequence_id )
    , net_uint< 8>, MEMBER1( Header::control_field )
    , net_int < 8>, MEMBER1( Header::log_message_interval )
>;

header_zipped default_zipped( Header );

int main()
{
    char buffer[128];
    Header header_src;
    Header header_dest;

    header_src.control_field = 157;
    header_src.correction_field = 134734235235ull;
    header_src.domain_number = 7;
    header_src.flag_field0   = 13;

    fill_random( header_src );

    make_aligned_ptr<4>( buffer ) << header_src;
    make_aligned_ptr<4>( buffer ) >> header_dest;

}
