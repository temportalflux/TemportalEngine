#ifndef INPUT_ACTION_HPP
#define INPUT_ACTION_HPP

#include "Namespace.h"
#include "types/integer.h"

NS_INPUT

enum class EAction : i32
{
	INVALID = -1,
	RELEASE = 0,
	PRESS = 1,
	REPEAT = 2,
};

NS_END

#endif // INPUT_ACTION_HPP