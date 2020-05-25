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

void Surface::releaseWindowHandle()
{
	mpWindowHandle = nullptr;
}

void Surface::swap(Surface &other)
{
	mpWindowHandle = other.mpWindowHandle;
	other.releaseWindowHandle();
	mSurface.swap(other.mSurface);
}

vk::Extent2D Surface::getDrawableSize() const
{
	// SDL returns drawable size in pixels in signed-integers (no idea why)
	i32 w, h;
	SDL_Window* pWindow = reinterpret_cast<SDL_Window*>(this->mpWindowHandle);
	SDL_Vulkan_GetDrawableSize(pWindow, &w, &h);
	return vk::Extent2D().setWidth((ui32)w).setHeight((ui32)h);
}

Surface& Surface::initialize(VulkanInstance const *pVulkan)
{
	assert(mpWindowHandle != nullptr);
	VkSurfaceKHR surface;
	SDL_Window* pWindow = reinterpret_cast<SDL_Window*>(this->mpWindowHandle);
	if (!SDL_Vulkan_CreateSurface(pWindow, (VkInstance)pVulkan->mInstance.get(), &surface))
	{
		//pVulkan->mLogger.log(logging::ECategory::LOGERROR, "Failed to create SDL Vulkan surface: %s", SDL_GetError());
		return *this;
	}
	mSurface = vk::UniqueSurfaceKHR(vk::SurfaceKHR(surface));
	return *this;
}

void Surface::destroy(VulkanInstance const *pVulkan)
{
	auto surface = mSurface.release();
	pVulkan->mInstance->destroySurfaceKHR(surface);
}
