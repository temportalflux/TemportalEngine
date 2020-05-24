#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/Surface.hpp"

NS_GRAPHICS
class VulkanInstance;

class VulkanRenderer
{

public:
	VulkanRenderer(VulkanInstance *pInstance, Surface &surface);

	void invalidate();

private:
	VulkanInstance *mpInstance;
	Surface mSurface;

	VulkanRenderer() = default;

};

NS_END
