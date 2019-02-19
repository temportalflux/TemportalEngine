#ifndef TYPES_INTEGER_H
#define TYPES_INTEGER_H

#include <stdio.h>

#include "arch.h"
#include "Namespace.h"


NS_TYPES

// signed integer types:
typedef __int8				i8;
typedef __int16				i16;
typedef __int32				i32;
typedef __int64				i64;

// unsigned integer types:
typedef unsigned __int8		ui8;
typedef unsigned __int16	ui16;
typedef unsigned __int32	ui32;
typedef unsigned __int64	ui64;

#ifdef __SIZEOF_INT128__
// huge integers
typedef __int128			i128;
typedef unsigned __int128	ui128;
typedef a3i128				hugeinteger;
typedef a3ui128				hugeindex;
#endif	// __SIZEOF_INT128__

typedef ui32 size;

// "pointer" as integer type
#if BIT_32
typedef ui32	ptr;
typedef i32		ptrDiff;
#else	// !BIT_32
typedef ui64	ptr;
typedef i64		ptrDiff;
#endif	// A3_32_BIT

NS_END

#endif //TYPES_INTEGER_H