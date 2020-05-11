#ifndef TE_EXECUTABLE_INFO_HPP
#define TE_EXECUTABLE_INFO_HPP

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// Libraries ------------------------------------------------------------------


// Engine ---------------------------------------------------------------------
#include "types/integer.h"

NS_UTILITY

struct SExecutableInfo
{
	char const * title;
	ui32 version;
};

NS_END

#endif