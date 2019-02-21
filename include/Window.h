#ifndef WINDOW_H
#define WINDOW_H

#include "types/integer.h"
#include "input/Event.hpp"

struct GLFWwindow;

class Window
{
public:
	typedef void (*DelegateKeyCallback)(Window *pWindow, struct input::Event const &evt);

	static void renderUntilClose(Window *pWindow);

private:
	uSize mWidth, mHeight;
	char const * mpTitle;

	GLFWwindow *mpHandle;
	DelegateKeyCallback mpDelegateKeyCallback;

public:
	Window() = default;
	Window(uSize width, uSize height, char const * title);

	bool isValid();

	void setKeyCallback(DelegateKeyCallback callback);
	void initializeRenderContext(int i);

	void markShouldClose();
	bool isClosePending();

	void pollInput();
	void render();
	void destroy();

private:

	void executeInputCallback(input::Event const &evt);
	friend void _callbackInternalKey(GLFWwindow *pWindowHandle,
		int key, int scancode, int action, int mods);
	friend void _callbackInternalMouseButton(GLFWwindow *pWindowHandle,
		int mouseButton, int action, int modifiers);
	friend void _callbackInternalScroll(GLFWwindow *pWindowHandle, double x, double y);

};

#endif //WINDOW_H
