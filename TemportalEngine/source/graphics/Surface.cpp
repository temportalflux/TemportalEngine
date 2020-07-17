#include "graphics/Surface.hpp"

#include "graphics/VulkanInstance.hpp"
#include "graphics/VulkanApi.hpp"

#include <SDL.h>
#include <SDL_vulkan.h>

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
	mInternal.swap(other.mInternal);
}

void* Surface::getWindowHandle() const
{
	return this->mpWindowHandle;
}

vk::Extent2D Surface::getDrawableSize() const
{
	// SDL returns drawable size in pixels in signed-integers (no idea why)
	i32 w, h;
	SDL_Window* pWindow = reinterpret_cast<SDL_Window*>(this->mpWindowHandle);
	SDL_Vulkan_GetDrawableSize(pWindow, &w, &h);
	return vk::Extent2D().setWidth((ui32)w).setHeight((ui32)h);
}

Surface& Surface::initialize(std::shared_ptr<VulkanInstance> pVulkan)
{
	assert(mpWindowHandle != nullptr);
	VkSurfaceKHR surface;
	SDL_Window* pWindow = reinterpret_cast<SDL_Window*>(this->mpWindowHandle);
	if (!SDL_Vulkan_CreateSurface(pWindow, graphics::extract<VkInstance>(pVulkan.get()), &surface))
	{
		//pVulkan->mLogger.log(logging::ECategory::LOGERROR, "Failed to create SDL Vulkan surface: %s", SDL_GetError());
		return *this;
	}
	mInternal = vk::UniqueSurfaceKHR(vk::SurfaceKHR(surface));
	return *this;
}

void* Surface::get()
{
	return &this->mInternal.get();
}

void Surface::destroy(std::shared_ptr<VulkanInstance> pVulkan)
{
	auto surface = mInternal.release();
	graphics::extract<vk::Instance>(pVulkan.get()).destroySurfaceKHR(surface);
}
