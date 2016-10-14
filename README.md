# netser - Network packet serialization
This lib was motivated when I wrote platform specific networt-packet serialization code. I'm no person that takes pride of having shifts, masking and hexadecimal constants everywhere. Added complication was that my microcontroller platform would not allow unaligned reads and writes. This lib aims to solve this, by generating only aligned reads and writes which are described in a platform-dependent list.

The main criteria for this lib are
- No bitshuffling code in protocol level sources
- Platform agnostic memory access patterns

The whole lib is built around aligned_ptr, packet field (type-)lists and structure description lists.

A motivational example of what this lib aims to provide:
```
	struct my_struct {
		unsigned int int_member1;
		unsigned int int_member2;
	};

	// Zipped Layout <-> Mapping definition: Describe the packet layout and its mapping to
    // structure members
	using zipped = netser::zipped< 
		net_uint<15>, MEMBER1(my_struct::int_member1),
		net_uint<17>, MEMBER1(my_struct::int_member2)
	>;
	
	// Allow netser to find the default zipped definition for my_struct
    // by providing an overload of the function "default_zipped" inside the
    // namespace my_struct lives in (ADL).
	zipped default_zipped(my_struct);
	
    void foo()
    {
		char buffer[32];
		my_struct obj;
		netser::make_aligned_ptr<1>(buffer) >> obj;  // read  "zipped" from 1-aligned buffer
		netser::make_aligned_ptr<2>(buffer) << obj;  // write "zipped" to 2-aligned buffer
    }
```

netser is tested with Visual Studio 2015 update 3, Visual Studio "15" preview, clang and ARM's current gcc port.

###aligned_ptr
The aligned_ptr template encapsulates a (packet-buffer-)pointer together with its alignment parameters and a valid-access-range:

	template< typename T, size_t Alignment, size_t Defect, typename AccessRange >
	struct aligned_ptr {...};

The pair {Alignment, Defect} together defines the alignment residue class the pointer value lives in. If the pointer f.e. is 4-byte aligned, this pair would be {4,0}. If on the other hand you know the pointer lives in a 4-byte aligned buffer but is at an offset that only has 2-byte alignment, you would state {4,2} instead, so the library knows that the offsets {+-2, +-6, ...} would be 4-byte aligned. _Note that this pair only describes guarantees on the supplied pointer_, it does not incure any runtime alignment operations.

The template parameter AccessRange can be used to tell the library more information of valid access offsets in the buffer pointed to by this pointer. The type AccessRange must contain a static constexpr member function

	static constexpr bool contains( int begin, int end );

which returns true iff an access beginning at "begin" and ending at "end" would be inside the range of the buffer pointed to by this pointer. There are two predefined implementations

	template< int Begin, int End >
	struct bounded;       // restrict accesses to the given range [Begin, End)
	
    template< int Begin >
    struct lower_bounded; // restrict accesses to offsets >= Begin

aligned_ptr defaults to using lower_bounded<0>, meaning the generated accesses cannot span over the beginning of the buffer.
Creating aligned_ptr's is made easy with the helper template function

	template< size_t Alignment, size_t Defect, int Offset, typename Bounds, typename Type >
    auto make_aligned_ptr( Type* ptr );

Alignment and Defect name the alignment guarantees for the passed pointer. Offset is applied to the pointer and the resulting Alignment and Defect are automatically updated, that is: {4, 0} + Offset 7 => { 4, 3 }.

Examples with explanation:

	void sample() {
    	char buffer[32];
        // describe a 4-byte aligned ptr.
        auto ptr1 = make_aligned_ptr<4>( buffer );

		// describe a pointer whose address when divided by 4 has the remainder 3.
        auto ptr2 = make_aligned_ptr<4,3>( &buffer[3] );

		// same as above, but also specify that netser can access the three bytes preceeding the pointer too.
        auto ptr3 = make_aligned_ptr<4,3,0,lower_bounded<-3>>( &buffer[3] );
        
        // equivalent short notation for above
        auto ptr3 = make_aligned_ptr<4,2,3>( buffer );
    }

###network packet layout
The most important part of this library is that it lets you define the memory layout of a network packet with a variadic type list of data fields:

	using mypacket = layout< field1, field2, field3, ... >;

The most important type of field is probably netser::int_. It defines an integer-field's size in bits, its byte order and its signedness. Since we're working on network packets, chances are you are only concerned with big-endian integer encodings, for which there are the netser::net_int<Size> and netser::net_uint<Size> aliases, and a few helpers for the common bit sizes 8, 16, 32, 64:

	using mypacket = layout< net_uint16, net_uint32, net_uint8, net_uint<4>, net_uint<4> >;

If you're facing arrays of a static size inside packets, you can decorate the fields with an array subscript:

	using arraypacket = layout< net_uint16[8] >;
    
It is not uncommon that you will face multiple different packet definitions which share commen sub-packets, that's why you can nest layouts too, you will probably use type aliases for this, however, written out:

    using mypacket = layout< layout< uint16, net_uint32 >, net_uint8 >;

###mappings
This library is about serializing and deserializing networks packets, so the question is, from or to where? netser uses structure mappings, which describe the host data representation of a network packet.
Consider that you have a layout and its corresponding c++ structure definition:
	
    using packet_layout = layout< net_uint32 >;
    
    struct packet {
    	unsigned int member;
    };

Now you want to serialize packet into a packet_layout or deserialize packet_layout into packet. That's when you have to define a mapping:

	using packet_mapping = mapping_list< mem< packet, unsigned int, &packet::member > >;
    
The order of member descriptors inside this mapping list is assumed to correspond to the order of fields in the packet layout description.

###memory accesses
One motivation for writing this library, beside avoiding hand crafted bit shifting, was that on some microcontroller platforms, namely a few ARM Cortex-M platforms, unaligned memory accesses will result in a runtime fault. This library is designed to respect the available memory accesses and their alignment requirements during serialization. One step to this goal was already taken with providing the aligned_ptr-template, which lets netser reason about what the alignment of the serialization buffer actually is. The other part to this goal is the definition of the platform memory access list. This list specifies the available memory accesses in size-ascending order, specifying their size and alignment requirements as well as their byte order. It must be provided in a file called "netser_config.hpp" and be customized to match your platform.
An exemplary definition:

		using platform_memory_accesses = type_list<
			atomic_memory_access< unsigned char,  1, 1, little_endian >,
			atomic_memory_access< unsigned short, 2, 2, little_endian >,
			atomic_memory_access< unsigned int,   4, 4, little_endian >,
			atomic_memory_access< uint64,         8, 8, little_endian >
		>;

All serialization and deserialization algorithms will consult this list of memory accesses when trying to figure out the best memory access pattern to execute their task.

### preliminary roundup
By this point we have everything that is necessary to serialize or deserialize a simple packet:
- Definition of aligned_ptr to reason about source or destination buffer alignment
- Definition of the packet layout
- Definition of the host structure
- Available memory accesses

With this in mind, we can already get our work done, here's an example:

	struct packet {
    	unsigned int member;
    };

	void sample()
    {
    	char buffer[4];
    	using mylayout  = layout< net_uint32 >; 
        using mymapping = mapping_list< mem< packet, unsigned int, &packet::member > >;

		packet thepacket;
		auto ptr = make_aligned_ptr<4>(buffer);
        
        read <mylayout, mymapping>(ptr, thepacket);	// deserialize
        write<mylayout, mymapping>(ptr, thepacket);	// serialize
    }

This example will probably look discouraging to you, it consists so many lines! But remember that the exact same code will work for _any_ alignment specified for the aligned_ptr. Also notice that you only have to describe the packet and the mapping once, you don't have to implement reading and writing seperately. It also works for sub-byte fields which span byte boundaries, etc..
What remains is that there's no handy way yet to define the payket layout and host structure mapping hand-in-hand, for when you have a direct relationshipt. That's where zipped definitions come in.

### zipped packet-mapping descriptions

Here's the example of the preceeding section, defined with a zipped mapping:

    using namespace netser;

	struct packet {
    	unsigned int member;
    };
    
    using zipped_packet = zipped<
    	net_uint32, MEMBER1(packet::member)
    >;

	zipped_packet default_zipped(packet);

	void sample()
    {
    	char buffer[4];
  
		packet thepacket;
		auto ptr = make_aligned_ptr<4>(buffer);
        
        ptr >> thepacket;
        ptr << thepacket;
    }

Now that there's a direct relationship specified between the host structure and the packet layout, netser can deduce the packet layout and structure mapping from the host type with the help of the overload of "default_zipped" you specifiy for your structure. This enables netser to provide the intuitive operator<< and operator>> overloads to read and write from or to a buffer pointed to by an aligned_ptr. Internally netser it will "unzip" the zipped mapping into a packet layout and a structure mapping and call the read and write template functions used in the preceeding section.

### final words
This is where this introductory finishes. Be aware that netser supports a few more operations like nested zip mappings and "reserved" fields inside packet layouts. You are hereby encouraged to peek into the unit tests for examples. netser itself is a header-only library and as such you only need to make the contents of the contained "include" directory available to your compiler's include paths. All identifiers are defined in the namespace "netser".