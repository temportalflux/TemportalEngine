#ifndef WINDOW_H
#define WINDOW_H

#include "types/integer.h"

struct GLFWwindow;

class Window
{
public:
	typedef void (*DelegateKeyCallback)(Window *pWindow, int key, int scancode, int action, int mods);

private:
	types::size mWidth, mHeight;
	char const * mpTitle;

	GLFWwindow *mpHandle;
	DelegateKeyCallback mpDelegateKeyCallback;

public:
	Window() = default;
	Window(types::size width, types::size height, char const * title);

	bool isValid();

	void setKeyCallback(DelegateKeyCallback callback);
	void initializeRenderContext(int i);

	void markShouldClose();
	bool isClosePending();

	void update();
	void destroy();

private:

	void executeKeyCallback(int key, int scancode, int action, int mods);
	friend void _keyCallbackInternal(GLFWwindow *pWindowHandle,
									 int key, int scancode, int action, int mods);

};

#endif //WINDOW_H
