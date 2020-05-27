#pragma once

#include "types/integer.h"

enum class WindowFlags : ui32
{
	NONE = 0,
	RESIZABLE = 1 << 0,
	RENDER_ON_THREAD = 1 << 1,
	BORDERLESS = 1 << 2,
};

inline WindowFlags operator&(WindowFlags lhs, WindowFlags rhs)
{
	return (WindowFlags)((ui32)lhs & (ui32)rhs);
}

inline WindowFlags operator|(WindowFlags lhs, WindowFlags rhs)
{
	return (WindowFlags)((ui32)lhs | (ui32)rhs);
}
