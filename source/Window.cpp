#include <Window.h>

#include <functional>
#include <GLFW/glfw3.h>

#include "logging/Logger.hpp"
#include "Engine.hpp"

void _keyCallbackInternal(GLFWwindow *pWindowHandle,
						  int key, int scancode, int action, int mods)
{
	void* userPtr = glfwGetWindowUserPointer(pWindowHandle);
	auto *pWindow = reinterpret_cast<Window *>(userPtr);
	pWindow->executeKeyCallback(key, scancode, action, mods);
}

void Window::renderUntilClose(Window * pWindow)
{
	while (pWindow->isValid() && !pWindow->isClosePending())
	{
		pWindow->render();
	}
}

Window::Window(uSize width, uSize height, char const * title)
	: mWidth(width)
	, mHeight(height)
	, mpTitle(title)
{
	this->mpHandle = glfwCreateWindow(mWidth, mHeight, mpTitle, nullptr, nullptr);
	if (!this->isValid())
	{
		DeclareLog("Window").log(logging::ECategory::ERROR,
				"Failed to create window");
	}

	glfwSetWindowUserPointer(this->mpHandle, this);
	glfwSetKeyCallback(this->mpHandle, &_keyCallbackInternal);
}

bool Window::isValid()
{
	return this->mpHandle != nullptr;
}

void Window::setKeyCallback(DelegateKeyCallback callback)
{
	this->mpDelegateKeyCallback = callback;
}

void Window::executeKeyCallback(int key, int scancode, int action, int mods)
{
	if (this->mpDelegateKeyCallback != nullptr)
	{
		(*this->mpDelegateKeyCallback)(this, key, scancode, action, mods);
	}
}

void Window::markShouldClose()
{
	glfwSetWindowShouldClose(this->mpHandle, GLFW_TRUE);
}

bool Window::isClosePending()
{
	return glfwWindowShouldClose(this->mpHandle) > 0;
}

void Window::pollInput()
{
	glfwPollEvents();
}

void Window::render()
{
	glfwSwapBuffers(this->mpHandle);
}

void Window::destroy()
{
	glfwDestroyWindow(this->mpHandle);
}

void Window::initializeRenderContext(int bufferSwapInterval)
{
	glfwMakeContextCurrent(this->mpHandle);
	//gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	glfwSwapInterval(bufferSwapInterval);
}
