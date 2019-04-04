#include "render/Renderer.hpp"
#include "Engine.hpp"
#include "memory/UniquePtr.hpp"

using namespace render;
//using namespace vk;

Renderer::Renderer()
	: maRequiredExtensionNames({
		"VK_KHR_surface",
		"VK_KHR_win32_surface",
	})
{

	mpInstanceInfo = memory::NewUnique<vk::InstanceCreateInfo>();
	//mpInstanceInfo->setPpEnabledExtensionNames(maRequiredExtensionNames.data());
	mpInstanceInfo->flags;

	//mpAppInstance = engine::Engine::Get()->alloc<vk::UniqueInstance>();
	//*((vk::UniqueInstance*)mpAppInstance) = vk::createInstanceUnique(*instanceInfo);

	
	

	/*
	mAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	mAppInfo.pNext = nullptr;
	mAppInfo.pApplicationName = "DemoGame";
	mAppInfo.pEngineName = "TemportalEngine";
	mAppInfo.engineVersion = 1;
	mAppInfo.applicationVersion = 1;
	mAppInfo.apiVersion = VK_API_VERSION_1_0;

	mAppInstanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	mAppInstanceInfo.pApplicationInfo = &mAppInfo;
	mAppInstanceInfo.pNext = nullptr;
	mAppInstanceInfo.enabledLayerCount = 0;
	mAppInstanceInfo.ppEnabledLayerNames = nullptr;
	mAppInstanceInfo.enabledExtensionCount = 2;
	mAppInstanceInfo.ppEnabledExtensionNames = maRequiredExtensionNames.data();
	mAppInstanceInfo.flags = 0;

	VkResult success = vkCreateInstance(&mAppInstanceInfo, nullptr, &mAppInstance);

	if (success == VK_SUCCESS)
	{
		LogEngineInfo("Vulkan is up and running!");
	}
	else
	{
		LogEngineInfo("Vulkan didnt work...");
		return;
	}

	vkEnumeratePhysicalDevices(mAppInstance, &mPhysicalDeviceCount, nullptr);
	assert(mPhysicalDeviceCount <= MAX_PHYSICAL_DEVICE_COUNT);
	vkEnumeratePhysicalDevices(mAppInstance, &mPhysicalDeviceCount, maVulkanPhysicalDevices);

	vkEnumerateInstanceExtensionProperties(nullptr, &mExtensionCount, nullptr);
	assert(mExtensionCount <= MAX_EXTENSION_COUNT);
	vkEnumerateInstanceExtensionProperties(nullptr, &mExtensionCount, maVulkanAvailableExtensions);
	*/
	
	

}

Renderer::~Renderer()
{
	//mPhysicalDeviceCount = 0;
	//mExtensionCount = 0;
	//vkDestroyInstance(mAppInstance, nullptr);
}

void Renderer::initializeWindow()
{

}

void Renderer::render()
{

}
