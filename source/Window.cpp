#include <Window.h>

#include <GLFW/glfw3.h>
#include <functional>

#include "dependency/GLFW.h"
#include "Log.h"

void _keyCallbackInternal(GLFWwindow *pWindowHandle,
						  int key, int scancode, int action, int mods)
{
	auto *pWindow = (Window *)glfwGetWindowUserPointer(pWindowHandle);
	pWindow->executeKeyCallback(key, scancode, action, mods);
}

Window::Window(types::size width, types::size height, char const * title)
	: mWidth(width)
	, mHeight(height)
	, mpTitle(title)
{
	this->mpHandle = glfwCreateWindow(mWidth, mHeight, mpTitle, nullptr, nullptr);
	if (!this->isValid())
	{
		logging::log(LogGlfw, logging::ECategory::ERROR,
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

void Window::update()
{
	glfwSwapBuffers(this->mpHandle);
	//glfwPollEvents();
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
