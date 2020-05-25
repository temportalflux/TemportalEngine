#pragma once

#include "dependency/SDL.hpp"
#include "types/integer.h"
#include "graphics/VulkanInstance.hpp"
#include "gui/GuiContext.hpp"

#include <unordered_map>
#include <vulkan/vulkan.hpp>

class Editor
{

public:
	logging::LogSystem LogSystem;

	Editor();
	~Editor();

	// it is assumed there is only ever 1 editor window
	void openWindow();
	void closeWindow();

	void createGui();

	void run();

private:
	dependency::SDL mDependencySDL[1];
	void* mpWindowHandle;

	static std::vector<const char*> VulkanValidationLayers;
	graphics::VulkanInstance mVulkanInstance;
	gui::GuiContext mGui;

	bool mIsRunning;

	bool initializeDependencies();
	void terminateDependencies();

	void createWindow();
	void destroyWindow();

	std::vector<const char*> querySDLVulkanExtensions() const;

	// VULKAN STUBBING - TO BE MOVED TO INTERNAL API
	void createFrameBuffers(i32 const width, i32 const height);

};
