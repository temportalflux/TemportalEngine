#ifndef MATH_COMPARE_H
#define MATH_COMPARE_H

// Honestly, this is straight up take from animal3D
// Full credit for macros goes to Dan TheMan Buckstein

#include "types/integer.h"

#define zero			0.0##f
#define one				1.0##f
#define te_pi				3.1415926535897932384626433832795##f
#define epsilon			1.19e-07##f
#define deg2rad			0.01745329251994329576923690768489##f
#define rad2deg			57.295779513082320876798154814105##f

#define isNearZero(n)			( (n) >= -epsilon && (n) <= epsilon )
#define isNotNearZero(n)		( (n) <  -epsilon || (n) >  epsilon )
#define a3recip(n)				( (n) != zero ? one / (n) : zero )

#define maximum(n0, n1)		( (n0) > (n1) ? (n0) : (n1) )
#define minimum(n0, n1)		( (n0) < (n1) ? (n0) : (n1) )
#define compare_absolute(n)	( (n) >= zero ? (n) : -(n) )
#define absoluteInt(n)		( (n) >= 0 ? (n) : -(n) )
#define clamp(n0, n1, v)	( (v) < (n1) ? (v) > (n0) ? (v) : (n0) : (n1) )
#define lerp(n0, n1, t)		( (n0) + ( (n1) - (n0) )*t )

// booleans
#ifndef __cplusplus
#define bool					ui8
#define true					1
#define false					0
#endif	// !__cplusplus

#endif