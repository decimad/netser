# netser
Network packet binary serialization

This lib has two motivations:
- Remove bit fiddling code from protocol level code
```
	struct my_struct {
		unsigned int int_member1;
		unsigned int int_member2;
	};

	// Zipped Layout <-> Mapping definition
	using zipped = netser::zipped< 
		net_uint<15>, MEMBER1(my_struct, int_member1) >,
		net_uint<17>, MEMBER2(my_struct, int_member2) >
	>;
	
	// Allow netser to find the default zipped definition for my_struct
	zipped default_zipped(my_struct);
	
	char buffer[32];
	my_struct obj;
	netser::make_aligned_ptr<1>(buffer) >> obj;  // read from 1-aligned buffer
	netser::make_aligned_ptr<2>(buffer) << obj;  // write to 2-aligned buffer
```
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
