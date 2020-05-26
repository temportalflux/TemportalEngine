#ifndef WINDOW_H
#define WINDOW_H

#include "TemportalEnginePCH.hpp"

#include "types/integer.h"
#include "input/Event.hpp"
#include "input/types.h"
#include "graphics/Surface.hpp"

#include <vector>

NS_INPUT
class Queue;
NS_END

NS_RENDER
class Renderer;
NS_END

NS_GRAPHICS
class VulkanInstance;
class VulkanRenderer;
NS_END

NS_UTILITY
struct SExecutableInfo;
NS_END

class TEMPORTALENGINE_API Window
{
private:
	ui16 mWidth, mHeight;
	char const * mpTitle;

	void* mpHandle;
	void* mpJoystick;
	bool mIsPendingClose;
	input::ListenerHandle mInputHandleQuit;

	graphics::VulkanRenderer *mpRenderer;

public:
	Window() = default;
	Window(
		ui16 width, ui16 height
	);
	void destroy();

	std::vector<const char*> querySDLVulkanExtensions() const;
	graphics::Surface createSurface() const;

	void setRenderer(graphics::VulkanRenderer *pRenderer);

	bool isValid();
	void addInputListeners(input::Queue *pQueue);

	void onInputQuit(input::Event const &evt);
	void markShouldClose();
	bool isPendingClose();

	/**
	 * Runs on the main thread as often as possible
	 * while the engine is running.
	 */
	void update();
	
	/**
	 * THREADED
	 * Executed in a near-infinite loop on the render thread.
	 * Tells the renderer to draw a frame.
	 * Returns true if the window is ready to close.
	 */
	bool renderUntilClose();

	/**
	 * THREADED
	 * Executes when the render thread has finished running.
	 * Waits for the renderer to be idle before exiting the thread.
	 */
	void waitForCleanup();

};

#endif //WINDOW_H
