#ifndef INPUT_EVENT_HPP
#define INPUT_EVENT_HPP

#include "TemportalEnginePCH.hpp"

// TODO: Organize Headers
#include "types/integer.h"
#include "Action.hpp"
#include "Key.hpp"
#include "Mouse.hpp"
#include "math/Vector.hpp"

NS_INPUT

enum class TEMPORTALENGINE_API EInputType : i32
{
	INVALID = -1,

	QUIT,
	KEY,
	MOUSE_MOVE,
	MOUSE_BUTTON,
	MOUSE_SCROLL,
	TEXT,

};

struct TEMPORTALENGINE_API Event
{
	EInputType type;

	union // MAX: 17B (17 bytes)
	{
		// Keyboard: key (4B), action (4B), modifiers (4B)
		struct // 12B
		{
			EAction action;
			EKeyModifier modifierMask;
			EKey key;
		} inputKey;

		// Mouse Btn: x, y, x relative, y relative (each 4B)
		struct // 16B
		{
			union
			{
				struct { f32 xDelta, yDelta; };
				math::Vector2 delta;
			};
			union
			{
				struct { i32 xCoord, yCoord; };
				math::Vector2Int coord;
			};
		} inputMouseMove;

		// Mouse Btn: key (4B), action (4B), clickCount (1B), x & y coord on screen (8B)
		struct // 17B
		{
			EAction action;
			EMouseButton button;
			ui8 clickCount;
			union
			{
				struct { i32 xCoord, yCoord; };
				math::Vector2Int coord;
			};
		} inputMouseButton;

		// Scroll: x-offset (4B), y-offset (4B)
		struct // 8B
		{
			union
			{
				struct { i32 xDelta, yDelta; };
				math::Vector2Int delta;
			};
		} inputScroll;

		struct
		{
			char text[32];
		} inputText;

	};

	Event() {}
	Event(Event const &other)
	{
		memcpy(this, &other, sizeof(*this));
	}

};

NS_END

#endif // INPUT_EVENT_HPP
