#pragma once

#include "TemportalEnginePCH.hpp"

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
