#include <Window.h>

#include <functional>
#include <GLFW/glfw3.h>

#include "logging/Logger.hpp"
#include "Engine.hpp"

void _callbackInternalKey(GLFWwindow *pWindowHandle,
	int key, int scancode, int action, int modifiers)
{
	void* userPtr = glfwGetWindowUserPointer(pWindowHandle);
	auto *pWindow = reinterpret_cast<Window *>(userPtr);

	auto evt = input::Event{ input::EInputType::KEY };
	evt.inputKey = {
		(input::EAction)action,
		(input::EKeyModifier)modifiers,
		(input::EKey)key,
	};
	pWindow->executeInputCallback(evt);
}

void _callbackInternalMouseButton(GLFWwindow *pWindowHandle,
	int mouseButton, int action, int modifiers)
{
	void* userPtr = glfwGetWindowUserPointer(pWindowHandle);
	auto *pWindow = reinterpret_cast<Window *>(userPtr);

	auto evt = input::Event{ input::EInputType::MOUSE_BUTTON };
	evt.inputMouseButton = {
		(input::EAction)action,
		(input::EKeyModifier)modifiers,
		(input::EMouseButton)mouseButton,
	};
	pWindow->executeInputCallback(evt);
}

void _callbackInternalScroll(GLFWwindow *pWindowHandle, double x, double y)
{
	void* userPtr = glfwGetWindowUserPointer(pWindowHandle);
	auto *pWindow = reinterpret_cast<Window *>(userPtr);

	auto evt = input::Event{ input::EInputType::SCROLL };
	evt.inputScroll = { x, y };
	pWindow->executeInputCallback(evt);
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
	glfwSetKeyCallback(this->mpHandle, &_callbackInternalKey);
	glfwSetMouseButtonCallback(this->mpHandle, &_callbackInternalMouseButton);
	glfwSetScrollCallback(this->mpHandle, &_callbackInternalScroll);
}

bool Window::isValid()
{
	return this->mpHandle != nullptr;
}

void Window::setKeyCallback(DelegateKeyCallback callback)
{
	this->mpDelegateKeyCallback = callback;
}

void Window::executeInputCallback(input::Event const &input)
{
	if (this->mpDelegateKeyCallback != nullptr)
	{
		(*this->mpDelegateKeyCallback)(this, input);
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
