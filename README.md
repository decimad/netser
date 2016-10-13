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
		net_uint<15>, MEMBER1(&my_struct::int_member1),
		net_uint<17>, MEMBER1(&my_struct::int_member2)
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

###aligned_ptr
The aligned_ptr template encapsulates a (packet-buffer-)pointer together with its alignment parameters and a valid-access-range:

	template< typename T, size_t Alignment, size_t Defect, typename AccessRange >
	struct aligned_ptr {...};

The pair {Alignment, Defect} together defines the alignment residue class the pointer value lives in. If the pointer f.e. is 4-byte aligned, this pair would be {4,0}. If on the other hand you know the pointer lives in a 4-byte aligned buffer but is at an offset that only has 2-byte alignment, you would state {4,2} instead, so the library knows that the offsets {+-2, +-6, ...} would be 4-byte aligned.

The template parameter AccessRange can be used to tell the library more information of valid access offsets in the buffer pointed to by this pointer. The type AccessRange must contain a static constexpr member function

	static constexpr bool contains( int begin, int end );

which returns true iff an access beginning at "begin" and ending at "end" would be inside the range of the buffer pointed to by this pointer. There are two predefined implementations

	template< int Begin, int End >
	struct two_side_limit_range;	// restrict accesses to the given range [Begin, End)
	
    template< int Begin >
    struct one_side_limit_range;	// restrict accesses to offsets >= Begin

aligned_ptr defaults to using one_side_limit_range<0>, meaning the generated accesses cannot span over the beginning of the buffer.

- Remove bit fiddling code from protocol level code

  Now from a definition standpoint, the list based approach works best for big-
  endian buffers, since their bit positioning does not depend on access sizes.
  That's why there's no special provision for non-byte spanning little-endian
  buffers, as I could not come up with a reasonable layout definition
  approach (yet?).
  There is also support for arrays and exploded arrays. The iterator-based
  internals also allow for dynamic sized fields, however none are implemented
  yet.
  
  ```operator>>(aligned_ptr, T) and operator<<(aligned_ptr, T)``` return a
  continuation-aligned_ptr, so you can buffer the result, check packet
  identifiers and read/write following content dependending on it. If all
  preceding layouts were fixed size, the underlying buffer pointer is never
  changed and the position inside the buffer is carried through the types only
  with offsets.
	
- Allow for quick adaption of platforms which don't allow unaligned memory accesses
  The ARM platform prominently does not support unaligned memory accesses. Depending
  on the net code, ip payload often will be 2-byte aligned or 4-byte aligned. You
  can program defensively with 2-byte in mind, or use this lib and just chang
  the alignment of the buffer pointer to the correct value. The generated read and
  write source will automatically adapt to produce the best aligned accesses.
