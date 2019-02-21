#ifndef INPUT_EVENT_HPP
#define INPUT_EVENT_HPP

#include "Api.h"

#include "Namespace.h"
#include "types/integer.h"
#include "Action.hpp"
#include "Key.hpp"
#include "Mouse.hpp"

NS_INPUT

enum class TEMPORTALENGINE_API EInputType : i32
{
	INVALID = -1,

	QUIT,
	KEY,
	MOUSE_MOVE,
	MOUSE_BUTTON,
	MOUSE_SCROLL,

};

struct TEMPORTALENGINE_API Event
{
	EInputType type;

	union
	{
		// Keyboard: key (4B), action (4B), modifiers (4B)
		struct // 12B
		{
			EAction action;
			EKeyModifier modifierMask;
			EKey key;
		} inputKey;

		// Mouse Btn: key (4B), action (4B), modifiers (4B)
		struct // 12B
		{
			EAction action;
			EKeyModifier modifierMask;
			EMouseButton button;
		} inputMouseButton;

		// Scroll: x-offset (8B), y-offset (8B)
		struct // 16B
		{
			double offsetX;
			double offsetY;
		} inputScroll;

	};
};

NS_END

#endif // INPUT_EVENT_HPP
