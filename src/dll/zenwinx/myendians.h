/*
 *		C++ definitions for checking endianness
 *
 * Copyright (c) 2007-2009 Jean-Pierre Andre
 *
 *	These classes should produce valid executable code, but
 *	they are just meant to ensure at compile-time there is no
 *	invalid action on endianness-dependent data.
 *
 *	All endianness-dependent data should be properly typed for
 *	these checks to be meaningfull.
 *
 *	This should be the first included file
 *
 *
 *			History
 *	Mar 2009
 * - first released version
 *
 *	Aug 2009
 * - added rejection of assignments to integer
 */

#ifndef _MYENDIANS_H
#define _MYENDIANS_H 1
#define _NTFS_ENDIANS_H  /* bar any further inclusion of "endians.h" */

/*
 *		Select applicable endianness
 */

#define __LITTLE_ENDIAN 1234
#define __BYTE_ORDER __LITTLE_ENDIAN

/*
 *		Use C++ standard boolean type
 */

#undef BOOL
#define BOOL bool

/*
 *		Define reversals
 */

#ifdef STSC
#define endian_rev16(x) ((((x) & 255L) << 8) + (((x) >> 8) & 255L))
#define endian_rev32(x) ((((x) & 255L) << 24) + (((x) & 0xff00L) << 8) \
		       + (((x) >> 8) & 0xff00L) + (((x) >> 24) & 255L))
#else
#define endian_rev16(x) ((((x) & 255) << 8) + (((x) >> 8) & 255))
#define endian_rev32(x) ((((x) & 255) << 24) + (((x) & 0xff00) << 8) \
		       + (((x) >> 8) & 0xff00) + (((x) >> 24) & 255))
#endif
#define endian_rev64(x) ((((x) & 255LL) << 56) + (((x) & 0xff00LL) << 40) \
		       + (((x) & 0xff0000LL) << 24) + (((x) & 0xff000000LL) << 8) \
		       + (((x) >> 8) & 0xff000000LL) + (((x) >> 24) & 0xff0000LL) \
		       + (((x) >> 40) & 0xff00LL) + (((x) >> 56) & 255LL))

/*
 *		Deal with compiler dialects
 */

#ifdef WNBC
#define longlong __int64
#define __attribute__(x)
#else
#define longlong long long
#endif

/*
 *		Define basic types based on types supported by compiler
 */

typedef char s8;
typedef short s16;
typedef longlong s64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned longlong u64;

#ifdef STSC
typedef long s32;
typedef unsigned long u32;
#else
typedef int s32;
typedef unsigned int u32;
#endif

/*
 *		Apply reversals according to cpu endianness
 */

#if __BYTE_ORDER == __LITTLE_ENDIAN

#define rev_be16(x) endian_rev16(x)
#define rev_be32(x) endian_rev32(x)

#define rev_le16(x) (x)
#define rev_le32(x) (x)
#define rev_le64(x) (x)

#else

#define rev_be16(x) (x)
#define rev_be32(x) (x)

#define rev_le16(x) endian_rev16(x)
#define rev_le32(x) endian_rev32(x)
#define rev_le64(x) endian_rev64(x)

#endif

/*
 *		Define endianness-dependent types as classes with
 *	controlled methods
 */

class illegal {  // for trapping illegal operands
	public :
		int v;
} ;


class le16 {
	unsigned short v;
	public :
		le16 makele16(int x) { le16 y; y.v = rev_le16(x); return (y);}
		le16 makele16(char x) { le16 y; y.v = rev_le16(x); return (y);}
		int getcpu(void) const { return (rev_le16(v)); }
		int operator == (const le16 x) const { return (x.v == v); }
		int operator != (const le16 x) const { return (x.v != v); }

		bool operator && (bool x) const { return (v && x); }
		bool operator || (bool x) const { return (v || x); }

		le16 operator & (const le16 x) const { le16 y; y.v = x.v & v; return (y); }
		le16 operator | (const le16 x) const { le16 y; y.v = x.v | v; return (y); }
		le16 operator ^ (const le16 x) const { le16 y; y.v = x.v ^ v; return (y); }
		le16 operator ~ (void) const { le16 y; y.v = ~v; return (y); }

		le16 operator &= (const le16 x) { v &= x.v; return (*this); }
		le16 operator |= (const le16 x) { v |= x.v; return (*this); }
		le16 operator ^= (const le16 x) { v ^= x.v; return (*this); }

		illegal operator + (int x) const { illegal y; y.v = x + v; return (y); }
		illegal operator - (int x) const { illegal y; y.v = x - v; return (y); }
		illegal operator & (int x) const { illegal y; y.v = x & v; return (y); }
		illegal operator | (int x) const { illegal y; y.v = x | v; return (y); }
		illegal operator ^ (int x) const { illegal y; y.v = x | v; return (y); }

		illegal operator < (const le16 x) const { illegal y; y.v = x.v < v; return (y); }
		illegal operator <= (const le16 x) const { illegal y; y.v = x.v <= v; return (y); }
		illegal operator > (const le16 x) const { illegal y; y.v = x.v > v; return (y); }
		illegal operator >= (const le16 x) const { illegal y; y.v = x.v >= v; return (y); }

		illegal operator == (int x) const { illegal y; y.v = x == v; return (y); }
		illegal operator != (int x) const { illegal y; y.v = x != v; return (y); }
		illegal operator < (int x) const { illegal y; y.v = x < v; return (y); }
		illegal operator <= (int x) const { illegal y; y.v = x <= v; return (y); }
		illegal operator > (int x) const { illegal y; y.v = x > v; return (y); }
		illegal operator >= (int x) const { illegal y; y.v = x >= v; return (y); }

		operator long long() const { illegal y; y.v = v; return (y.v); }
		operator long() const { illegal y; y.v = v; return (y.v); }
		operator int() const { illegal y; y.v = v; return (y.v); }
		operator short() const { illegal y; y.v = v; return (y.v); }
		operator bool() const  { return (v != 0); }
		bool operator !() const  { return (v == 0); }
} __attribute__((__packed__));

class be16 {
	unsigned short v;
	public :
		be16 makebe16(int x) { be16 y; y.v = rev_be16(x); return (y);}
		be16 makebe16(char x) { be16 y; y.v = rev_be16(x); return (y);}
		int getcpu(void) const { return (rev_be16(v)); }
		int operator == (const be16 x) const { return (x.v == v); }
		int operator != (const be16 x) const { return (x.v != v); }

		bool operator && (bool x) const { return (v && x); }
		bool operator || (bool x) const { return (v || x); }

		be16 operator & (const be16 x) const { be16 y; y.v = x.v & v; return (y); }
		be16 operator | (const be16 x) const { be16 y; y.v = x.v | v; return (y); }
		be16 operator ^ (const be16 x) const { be16 y; y.v = x.v ^ v; return (y); }
		be16 operator ~ (void) const { be16 y; y.v = ~v; return (y); }

		be16 operator &= (const be16 x) { v &= x.v; return (*this); }
		be16 operator |= (const be16 x) { v |= x.v; return (*this); }
		be16 operator ^= (const be16 x) { v ^= x.v; return (*this); }

		illegal operator + (int x) const { illegal y; y.v = x + v; return (y); }
		illegal operator - (int x) const { illegal y; y.v = x - v; return (y); }
		illegal operator & (int x) const { illegal y; y.v = x & v; return (y); }
		illegal operator | (int x) const { illegal y; y.v = x | v; return (y); }
		illegal operator ^ (int x) const { illegal y; y.v = x | v; return (y); }

		illegal operator < (const be16 x) const { illegal y; y.v = x.v < v; return (y); }
		illegal operator <= (const be16 x) const { illegal y; y.v = x.v <= v; return (y); }
		illegal operator > (const be16 x) const { illegal y; y.v = x.v > v; return (y); }
		illegal operator >= (const be16 x) const { illegal y; y.v = x.v >= v; return (y); }

		illegal operator == (int x) const { illegal y; y.v = x == v; return (y); }
		illegal operator != (int x) const { illegal y; y.v = x != v; return (y); }
		illegal operator < (int x) const { illegal y; y.v = x < v; return (y); }
		illegal operator <= (int x) const { illegal y; y.v = x <= v; return (y); }
		illegal operator > (int x) const { illegal y; y.v = x > v; return (y); }
		illegal operator >= (int x) const { illegal y; y.v = x >= v; return (y); }

		operator long long() const { illegal y; y.v = v; return (y.v); }
		operator long() const { illegal y; y.v = v; return (y.v); }
		operator int() const { illegal y; y.v = v; return (y.v); }
		operator short() const { illegal y; y.v = v; return (y.v); }
		operator bool() const  { return (v != 0); }
		bool operator !() const  { return (v == 0); }
} __attribute__((__packed__));

class le32 {
	unsigned int v;
	public :
		le32 makele32(int x) { le32 y; y.v = rev_le32(x); return (y);}
		long getcpu(void) const { return (rev_le32(v)); }
		int operator == (const le32 x) const { return (x.v == v); }
		int operator != (const le32 x) const { return (x.v != v); }

		bool operator && (bool x) const { return (v && x); }
		bool operator || (bool x) const { return (v || x); }

		le32 operator & (const le32 x) const { le32 y; y.v = x.v & v; return (y); }
		le32 operator | (const le32 x) const { le32 y; y.v = x.v | v; return (y); }
		le32 operator ^ (const le32 x) const { le32 y; y.v = x.v ^ v; return (y); }
		le32 operator ~ (void) const { le32 y; y.v = ~v; return (y); }

		le32 operator &= (const le32 x) { v &= x.v; return (*this); }
		le32 operator |= (const le32 x) { v |= x.v; return (*this); }
		le32 operator ^= (const le32 x) { v ^= x.v; return (*this); }

		illegal operator + (int x) const { illegal y; y.v = x + v; return (y); }
		illegal operator - (int x) const { illegal y; y.v = x - v; return (y); }
		illegal operator & (int x) const { illegal y; y.v = x & v; return (y); }
		illegal operator | (int x) const { illegal y; y.v = x | v; return (y); }
		illegal operator ^ (int x) const { illegal y; y.v = x | v; return (y); }

		illegal operator < (const le32 x) const { illegal y; y.v = x.v < v; return (y); }
		illegal operator <= (const le32 x) const { illegal y; y.v = x.v <= v; return (y); }
		illegal operator > (const le32 x) const { illegal y; y.v = x.v > v; return (y); }
		illegal operator >= (const le32 x) const { illegal y; y.v = x.v >= v; return (y); }

		illegal operator == (int x) const { illegal y; y.v = x == v; return (y); }
		illegal operator != (int x) const { illegal y; y.v = x != v; return (y); }
		illegal operator < (int x) const { illegal y; y.v = x < v; return (y); }
		illegal operator <= (int x) const { illegal y; y.v = x <= v; return (y); }
		illegal operator > (int x) const { illegal y; y.v = x > v; return (y); }
		illegal operator >= (int x) const { illegal y; y.v = x >= v; return (y); }

		operator long long() const { illegal y; y.v = v; return (y.v); }
		operator long() const { illegal y; y.v = v; return (y.v); }
		operator int() const { illegal y; y.v = v; return (y.v); }
		operator short() const { illegal y; y.v = v; return (y.v); }
		operator bool() const  { return (v != 0); }
		bool operator !() const  { return (v == 0); }
} __attribute__((__packed__));

class be32 {
	unsigned int v;
	public :
		be32 makebe32(int x) { be32 y; y.v = rev_be32(x); return (y);}
		long getcpu(void) const { return (rev_be32(v)); }
		int operator == (const be32 x) const { return (x.v == v); }
		int operator != (const be32 x) const { return (x.v != v); }

		bool operator && (bool x) const { return (v && x); }
		bool operator || (bool x) const { return (v || x); }

		be32 operator & (const be32 x) const { be32 y; y.v = x.v & v; return (y); }
		be32 operator | (const be32 x) const { be32 y; y.v = x.v | v; return (y); }
		be32 operator ^ (const be32 x) const { be32 y; y.v = x.v ^ v; return (y); }
		be32 operator ~ (void) const { be32 y; y.v = ~v; return (y); }

		be32 operator &= (const be32 x) { v &= x.v; return (*this); }
		be32 operator |= (const be32 x) { v |= x.v; return (*this); }
		be32 operator ^= (const be32 x) { v ^= x.v; return (*this); }

		illegal operator + (int x) const { illegal y; y.v = x + v; return (y); }
		illegal operator - (int x) const { illegal y; y.v = x - v; return (y); }
		illegal operator & (int x) const { illegal y; y.v = x & v; return (y); }
		illegal operator | (int x) const { illegal y; y.v = x | v; return (y); }
		illegal operator ^ (int x) const { illegal y; y.v = x | v; return (y); }

		illegal operator < (const be32 x) const { illegal y; y.v = x.v < v; return (y); }
		illegal operator <= (const be32 x) const { illegal y; y.v = x.v <= v; return (y); }
		illegal operator > (const be32 x) const { illegal y; y.v = x.v > v; return (y); }
		illegal operator >= (const be32 x) const { illegal y; y.v = x.v >= v; return (y); }

		illegal operator == (int x) const { illegal y; y.v = x == v; return (y); }
		illegal operator != (int x) const { illegal y; y.v = x != v; return (y); }
		illegal operator < (int x) const { illegal y; y.v = x < v; return (y); }
		illegal operator <= (int x) const { illegal y; y.v = x <= v; return (y); }
		illegal operator > (int x) const { illegal y; y.v = x > v; return (y); }
		illegal operator >= (int x) const { illegal y; y.v = x >= v; return (y); }

		operator long long() const { illegal y; y.v = v; return (y.v); }
		operator long() const { illegal y; y.v = v; return (y.v); }
		operator int() const { illegal y; y.v = v; return (y.v); }
		operator short() const { illegal y; y.v = v; return (y.v); }
		operator bool() const  { return (v != 0); }
		bool operator !() const  { return (v == 0); }
} __attribute__((__packed__));

class le64 {
	longlong v;
	public :
		le64 makele64(int x) { le64 y; y.v = rev_le64(x); return (y);}
		le64 makele64(longlong x) { le64 y; y.v = rev_le64(x); return (y);}
		longlong getcpu(void) const { return (v); }
		int operator == (const le64 x) const { return (x.v == v); }
		int operator != (const le64 x) const { return (x.v != v); }

		bool operator && (bool x) const { return (v && x); }
		bool operator || (bool x) const { return (v || x); }

		le64 operator & (const le64 x) const { le64 y; y.v = x.v & v; return (y); }
		le64 operator | (const le64 x) const { le64 y; y.v = x.v | v; return (y); }
		le64 operator ^ (const le64 x) const { le64 y; y.v = x.v ^ v; return (y); }
		le64 operator ~ (void) const { le64 y; y.v = ~v; return (y); }

		le64 operator &= (const le64 x) { v &= x.v; return (*this); }
		le64 operator |= (const le64 x) { v |= x.v; return (*this); }
		le64 operator ^= (const le64 x) { v ^= x.v; return (*this); }

		illegal operator + (int x) const { illegal y; y.v = x + v; return (y); }
		illegal operator - (int x) const { illegal y; y.v = x - v; return (y); }
		illegal operator & (int x) const { illegal y; y.v = x & v; return (y); }
		illegal operator | (int x) const { illegal y; y.v = x | v; return (y); }
		illegal operator ^ (int x) const { illegal y; y.v = x | v; return (y); }

		illegal operator < (const le64 x) const { illegal y; y.v = x.v < v; return (y); }
		illegal operator <= (const le64 x) const { illegal y; y.v = x.v <= v; return (y); }
		illegal operator > (const le64 x) const { illegal y; y.v = x.v > v; return (y); }
		illegal operator >= (const le64 x) const { illegal y; y.v = x.v >= v; return (y); }

		illegal operator == (int x) const { illegal y; y.v = x == v; return (y); }
		illegal operator != (int x) const { illegal y; y.v = x != v; return (y); }
		illegal operator < (int x) const { illegal y; y.v = x < v; return (y); }
		illegal operator <= (int x) const { illegal y; y.v = x <= v; return (y); }
		illegal operator > (int x) const { illegal y; y.v = x > v; return (y); }
		illegal operator >= (int x) const { illegal y; y.v = x >= v; return (y); }

		operator long long() const { illegal y; y.v = v; return (y.v); }
		operator long() const { illegal y; y.v = v; return (y.v); }
		operator int() const { illegal y; y.v = v; return (y.v); }
		operator short() const { illegal y; y.v = v; return (y.v); }
		operator bool() const  { return (v != 0); }
		bool operator !() const  { return (v == 0); }
} __attribute__((__packed__));

typedef class le16 sle16;
typedef class le32 sle32;
typedef class le64 sle64;

/*
 *		Define conversion functions or macroes
 */

le16 cpu_to_le16(int x) { le16 y; return (y.makele16(x)); }
le32 cpu_to_le32(int x) { le32 y; return (y.makele32(x)); }
le64 cpu_to_le64(longlong x) { le64 y; return (y.makele64(x)); }
le64 cpu_to_sle64(longlong x) { le64 y; return (y.makele64(x)); }

be16 cpu_to_be16(int x) { be16 y; return (y.makebe16(x)); }
be32 cpu_to_be32(int x) { be32 y; return (y.makebe32(x)); }


#define be16_to_cpu(x) ((x).getcpu())
#define le16_to_cpu(x) ((x).getcpu())
#define le16_to_cpup(p) ((p)->getcpu())
#define be32_to_cpu(x) ((x).getcpu())
#define le32_to_cpu(x) ((x).getcpu())
#define le32_to_cpup(p) ((p)->getcpu())
#define le64_to_cpu(x) ((x).getcpu())
#define le64_to_cpup(p) ((p)->getcpu())
#define sle64_to_cpu(x) ((x).getcpu())
#define sle64_to_cpup(x) ((x)->getcpu())

#define const_le16_to_cpu(x) le16_to_cpu(x)
#define const_le32_to_cpu(x) le32_to_cpu(x)
#define const_le64_to_cpu(x) le64_to_cpu(x)
#define const_cpu_to_le16(x) cpu_to_le16(x)
#define const_cpu_to_le32(x) cpu_to_le32(x)
#define const_cpu_to_le64(x) cpu_to_le64(x)
#define const_cpu_to_sle64(x) cpu_to_sle64(x)
#define const_cpu_to_be16(x) cpu_to_be16(x)
#define const_cpu_to_be32(x) cpu_to_be32(x)

#undef endian_rev16
#undef endian_rev32
#undef endian_rev64

#endif /* _MYENDIANS_H */
