#ifndef WINDOW_H
#define WINDOW_H

#include "Api.h"

#include "types/integer.h"
#include "input/Event.hpp"

class TEMPORTALENGINE_API Window
{
public:
	typedef void (*DelegateKeyCallback)(Window *pWindow, struct input::Event const &evt);

	static void renderUntilClose(void* ptr);

private:
	uSize mWidth, mHeight;
	char const * mpTitle;

	struct SDL_Window *mpHandle;
	bool mIsPendingClose;

	DelegateKeyCallback mpDelegateInputCallback;

public:
	Window() = default;
	Window(uSize width, uSize height, char const * title);

	bool isValid();

	void setInputCallback(DelegateKeyCallback callback);
	void initializeRenderContext(int i);

	void markShouldClose();
	bool isPendingClose();

	void pollInput();
	void render();
	void destroy();

private:

	void executeInputCallback(input::Event const &evt);

};

#endif //WINDOW_H
