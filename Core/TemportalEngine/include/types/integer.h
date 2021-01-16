#ifndef TYPES_INTEGER_H
#define TYPES_INTEGER_H

#include "arch.h"

// signed integer types:
typedef __int8				i8;  // char, 8 bits, 1 byte
typedef __int16				i16; // short, 16 bits, 2 bytes
typedef __int32				i32; // int, 32 bits, 4 bytes
typedef __int64				i64; // long long, 64 bits, 8 bytes

// unsigned integer types:
typedef unsigned __int8		ui8;  // unsigned char, 8 bits, 1 byte
typedef unsigned __int16	ui16; // unsigned short, 16 bits, 2 bytes
typedef unsigned __int32	ui32; // unsigned int, 32 bits, 4 bytes
typedef unsigned __int64	ui64; // unsigned long long, 64 bits, 8 bytes

#ifdef __SIZEOF_INT128__
// huge integers
typedef __int128			i128;
typedef unsigned __int128	ui128;
typedef a3i128				hugeinteger;
typedef a3ui128				hugeindex;
#endif	// __SIZEOF_INT128__

#ifdef _WIN64
typedef ui64 uSize;
#else
typedef ui32 uSize;
#endif

typedef uSize uIndex;

// "pointer" as integer type
#if BIT_32
typedef ui32	ptr;
typedef i32		ptrDiff;
#else	// !BIT_32
typedef ui64	ptr;
typedef i64		ptrDiff;
#endif	// A3_32_BIT

#endif // TYPES_INTEGER_H