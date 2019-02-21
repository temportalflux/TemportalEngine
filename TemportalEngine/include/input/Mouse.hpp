#ifndef INPUT_MOUSE_HPP
#define INPUT_MOUSE_HPP

#include "Api.h"

#include "Namespace.h"
#include "types/integer.h"

NS_INPUT

enum class TEMPORTALENGINE_API EMouseButton : i32
{
	INVALID = -1,
	BUTTON_1 = 0,
	BUTTON_2 = 1,
	BUTTON_3 = 2,
	BUTTON_4 = 3,
	BUTTON_5 = 4,
	BUTTON_6 = 5,
	BUTTON_7 = 6,
	BUTTON_8 = 7,
	BUTTON_LAST = BUTTON_8,
	BUTTON_LEFT = BUTTON_1,
	BUTTON_RIGHT = BUTTON_2,
	BUTTON_MIDDLE = BUTTON_3,
};

NS_END

#endif // INPUT_MOUSE_HPP