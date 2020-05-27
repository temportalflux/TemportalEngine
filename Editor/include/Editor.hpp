#pragma once

#include "TemportalEnginePCH.hpp"

//#include "dependency/SDL.hpp"
//#include "types/integer.h"
//#include "graphics/VulkanInstance.hpp"
//#include "gui/GuiContext.hpp"

//#include <unordered_map>
//#include <vulkan/vulkan.hpp>

NS_ENGINE
class Engine;
NS_END

NS_GRAPHICS
class VulkanRenderer;
NS_END

class Editor
{

public:

	Editor();
	~Editor();

	bool setup();
	void run();

private:
	void initializeRenderer(graphics::VulkanRenderer *pRenderer);

};
