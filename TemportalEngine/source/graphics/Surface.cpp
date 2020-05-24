#include "graphics/Surface.hpp"

#include "graphics/VulkanInstance.hpp"

#include <SDL.h>
#include <SDL_vulkan.h>
#include "..\..\include\graphics\Surface.hpp"

using namespace graphics;

Surface::Surface(void *pWindowHandle)
	: mpWindowHandle(pWindowHandle)
{
}

void Surface::swap(Surface &other)
{
	mpWindowHandle = other.mpWindowHandle;
	other.mpWindowHandle = nullptr;
	mSurface.swap(other.mSurface);
}

void Surface::releaseWindowHandle()
{
	mpWindowHandle = nullptr;
}

Surface& Surface::initialize(VulkanInstance *pVulkan)
{
	assert(mpWindowHandle != nullptr);
	VkSurfaceKHR surface;
	SDL_Window* pWindow = reinterpret_cast<SDL_Window*>(this->mpWindowHandle);
	if (!SDL_Vulkan_CreateSurface(pWindow, (VkInstance)pVulkan->mInstance.get(), &surface))
	{
		pVulkan->mLogger.log(logging::ECategory::LOGERROR, "Failed to create SDL Vulkan surface: %s", SDL_GetError());
		return *this;
	}
	mSurface = vk::UniqueSurfaceKHR(vk::SurfaceKHR(surface));
	return *this;
}

void Surface::destroy(VulkanInstance *pVulkan)
{
	auto surface = mSurface.release();
	pVulkan->mInstance->destroySurfaceKHR(surface);
}
