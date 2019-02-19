#include <GLFW/glfw3.h>
#include "dependency/Dependency.h"
#include "dependency/GLFW.h"
#include "Window.h"
#include "Log.h"
#include "Thread.h"

using namespace std;

#define LogEngine "TemportalEngine"

static void key_callback(Window* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		window->markShouldClose();
	}
}

static void updateWindow(Window* pWindow)
{
	while (!pWindow->isClosePending()) pWindow->update();
}

int main()
{
	logging::log(LogEngine, logging::ECategory::INFO, "Hello World!");

	GLFW pDepGlfw[1];
	*pDepGlfw = GLFW();

	if (!pDepGlfw->initialize()) return 1;

	Window pWindow[1];
	*pWindow = Window(640, 480, "Temportal Engine");
	if (!pWindow->isValid())
	{
		pDepGlfw->terminate();
		return 1;
	}

	pWindow->setKeyCallback(key_callback);

	pWindow->initializeRenderContext(1);;

	//Thread<Window*> pThread[1];
	//*pThread = Thread<Window*>("Window Updater");
	//pThread->start(&updateWindow, pWindow);
	//pThread->join();
	updateWindow(pWindow);

	pWindow->destroy();
	pDepGlfw->terminate();

	return 0;
}