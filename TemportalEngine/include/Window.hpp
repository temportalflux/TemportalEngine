#ifndef WINDOW_H
#define WINDOW_H

#include "TemportalEnginePCH.hpp"

#include "types/integer.h"
#include "input/Event.hpp"

// TODO: Namespace
// TODO: Organize Headers

class TEMPORTALENGINE_API Window
{
public:
	static void renderUntilClose(void* ptr);

private:
	uSize mWidth, mHeight;
	char const * mpTitle;

	void* mpHandle;
	void* mpJoystick;
	bool mIsPendingClose;

public:
	Window() = default;
	Window(uSize width, uSize height, char const * title);

	bool isValid();

	void initializeRenderContext(int i);

	void markShouldClose();
	bool isPendingClose();

	void render();
	void destroy();

};

#endif //WINDOW_H
