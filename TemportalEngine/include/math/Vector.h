#ifndef MATH_VECTOR_H
#define MATH_VECTOR_H

#include "types/integer.h"
#include "types/real.h"

struct Vector2Raw
{

	/**
	INVALID = -1
	REAL = 0
	INT = 1
	UINT = 2
	*/
	i8 type;

	union
	{
		f32 realSet[2];
		struct
		{
			f32 x, y;
		} real;

		struct
		{
			i32 x, y;
		} sint;
		i32 intSet[2];

		struct
		{
			ui32 x, y;
		} uint;
		ui32 uintSet[2];
	} value;

};

struct Vector3Raw
{

	/**
	INVALID = -1
	REAL = 0
	INT = 1
	UINT = 2
	*/
	i8 type;

	union
	{
		f32 realSet[3];
		struct
		{
			f32 x, y, z;
		} real;

		struct
		{
			i32 x, y, z;
		} sint;
		i32 intSet[3];

		struct
		{
			ui32 x, y, z;
		} uint;
		ui32 uintSet[3];
	} value;

};

struct Vector4Raw
{

	union
	{
		f32 realSet[4];
		struct
		{
			f32 x, y, z, w;
		} real;
	} value;

};

#endif