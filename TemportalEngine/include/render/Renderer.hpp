#ifndef TE_RENDERER_HPP
#define TE_RENDERER_HPP
#pragma warning(push)
#pragma warning(disable:4251) // disable STL warnings in dll

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// Libraries ------------------------------------------------------------------
#include <vulkan/vulkan.hpp>
#include <array>

// Engine ---------------------------------------------------------------------
#include "types/integer.h"
#include "input/Event.hpp"
#include "memory/UniquePtr.hpp"
//#include "math/pow.hpp"

NS_RENDER

class TEMPORTALENGINE_API Renderer
{
	typedef char const* CSTR;

private:

	memory::SharedPtrOpt<vk::InstanceCreateInfo> mpInstanceInfo;
	void* mpAppInstance;

	static ui8 const REQUIRED_EXTENSION_COUNT = 2;
	std::array<CSTR, REQUIRED_EXTENSION_COUNT> maRequiredExtensionNames;

	/*
	static uSize const MAX_PHYSICAL_DEVICE_COUNT = 4; // Max GPUs
	uSize mPhysicalDeviceCount;
	VkPhysicalDevice maVulkanPhysicalDevices[MAX_PHYSICAL_DEVICE_COUNT];
	//*/

	/*
	constexpr static uSize const MAX_EXTENSION_COUNT = 512;// pow<ui32, 2, 32>::value - 1;
	uSize mExtensionCount;
	VkExtensionProperties maVulkanAvailableExtensions[MAX_EXTENSION_COUNT];
	//*/

public:
	Renderer();
	~Renderer();

	void initializeWindow();
	void render();

};

NS_END

#pragma warning(pop)
#endif
