#ifndef INPUT_MOUSE_HPP
#define INPUT_MOUSE_HPP

#include "Api.h"

#include "Namespace.h"
#include "types/integer.h"

NS_INPUT

enum class TEMPORTALENGINE_API EMouseButton : i32
{
	INVALID = 0,
	BUTTON_1 = 1,
	BUTTON_2 = 2,
	BUTTON_3 = 3,
	BUTTON_4 = 4,
	BUTTON_5 = 5,
	BUTTON_6 = 6,
	BUTTON_7 = 7,
	BUTTON_8 = 8,
};

NS_END

#endif // INPUT_MOUSE_HPP