#include "graphics/Surface.hpp"

#include "graphics/VulkanInstance.hpp"

#include <SDL.h>
#include <SDL_vulkan.h>

using namespace graphics;

Surface::Surface(void *pWindowHandle)
	: mpWindowHandle(pWindowHandle)
{
}

void Surface::create(VulkanInstance *pVulkan)
{
	VkSurfaceKHR surface;
	SDL_Window* pWindow = reinterpret_cast<SDL_Window*>(this->mpWindowHandle);
	if (!SDL_Vulkan_CreateSurface(pWindow, pVulkan->mInstance.get(), &surface))
	{
		pVulkan->mLogger.log(logging::ECategory::LOGERROR, "Failed to create SDL Vulkan surface: %s", SDL_GetError());
		return;
	}
	mSurface = vk::UniqueSurfaceKHR(vk::SurfaceKHR(surface));
}

void Surface::destroy(VulkanInstance *pVulkan)
{
	auto surface = mSurface.release();
	pVulkan->mInstance->destroySurfaceKHR(surface);
}
