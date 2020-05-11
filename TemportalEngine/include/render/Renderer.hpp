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
#include "memory/SharedPtr.hpp"
//#include "math/pow.hpp"

NS_UTILITY
struct SExecutableInfo;
NS_END

NS_RENDER

class TEMPORTALENGINE_API Renderer
{
	typedef char const* CSTR;

private:

	vk::ApplicationInfo mpApplicationInfo[1];
	vk::InstanceCreateInfo mpInstanceInfo[1];
	memory::AllocatedPtr<vk::UniqueInstance> mpAppInstance;

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
	Renderer(
		utility::SExecutableInfo const *const appInfo,
		utility::SExecutableInfo const *const engineInfo
	);
	~Renderer();

	void initializeWindow();
	void render();

};

NS_END

#pragma warning(pop)
#endif
