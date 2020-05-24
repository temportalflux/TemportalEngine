#include "graphics/VulkanRenderer.hpp"

#include "graphics/VulkanInstance.hpp"

using namespace graphics;

VulkanRenderer::VulkanRenderer(VulkanInstance *pInstance, Surface &surface)
	: mpInstance(pInstance)
{
	mSurface.swap(surface);

	/*
	auto optPhysicalDevice = pVulkan->pickPhysicalDevice(
		graphics::PhysicalDevicePreference()
		.addCriteriaQueueFamily(graphics::QueueFamily::eGraphics),
		&surface
	);
	if (!optPhysicalDevice.has_value())
	{
		pVulkan->getLog().log(logging::ECategory::LOGERROR, "Failed to find a suitable GPU/physical device.");
		engine::Engine::Destroy();
		return 1;
	}
	//*/
}

void VulkanRenderer::invalidate()
{
	mSurface.destroy(mpInstance);
	mSurface.releaseWindowHandle();

	mpInstance = nullptr;
}
