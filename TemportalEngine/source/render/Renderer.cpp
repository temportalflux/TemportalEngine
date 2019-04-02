#include "render/Renderer.hpp"
#include <vulkan/vulkan.h>
//#include <vulkan/vulkan.hpp>
#include "Engine.hpp"

using namespace render;
//using namespace vk;

Renderer::Renderer()
{

	VkInstance AppInstance;
	VkInstanceCreateInfo InstanceCreateInfo;
	VkApplicationInfo AppInfo;

	AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	AppInfo.pNext = nullptr;
	AppInfo.pApplicationName = "DemoGame";
	AppInfo.pEngineName = "TemportalEngine";
	AppInfo.engineVersion = 1;
	AppInfo.applicationVersion = 1;
	AppInfo.apiVersion = VK_API_VERSION_1_0;

	InstanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	InstanceCreateInfo.pNext = nullptr;
	InstanceCreateInfo.enabledLayerCount = 0;
	InstanceCreateInfo.enabledExtensionCount = 0;
	InstanceCreateInfo.flags = 0;
	InstanceCreateInfo.ppEnabledExtensionNames = nullptr;
	InstanceCreateInfo.ppEnabledLayerNames = nullptr;

	InstanceCreateInfo.pApplicationInfo = &AppInfo;

	VkResult success = vkCreateInstance(&InstanceCreateInfo, nullptr, &AppInstance);

	if (success == VK_SUCCESS)
	{
		LogEngineInfo("Vulkan is up and running!");
	}
	else
	{
		LogEngineInfo("Vulkan didnt work...");
		return;
	}

	ui32 physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(AppInstance, &physicalDeviceCount, nullptr);
	VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[physicalDeviceCount];
	//VkPhysicalDevice* physicalDevices = engine::Engine::Get()->allocArray<VkPhysicalDevice>(physicalDeviceCount);
	vkEnumeratePhysicalDevices(AppInstance, &physicalDeviceCount, physicalDevices);

	ui32 extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	VkExtensionProperties *extensionsAvailable = new VkExtensionProperties[extensionCount];
	//VkExtensionProperties *extensionsAvailable = engine::Engine::Get()->allocArray<VkExtensionProperties>(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionsAvailable);

	//engine::Engine::Get()->deallocArray(physicalDeviceCount, &physicalDevices);
	delete[] physicalDevices;
	//engine::Engine::Get()->deallocArray(extensionCount, &extensionsAvailable);
	delete[] extensionsAvailable;

	vkDestroyInstance(AppInstance, nullptr);
}

Renderer::~Renderer()
{
}

void Renderer::initializeWindow()
{

}

void Renderer::render()
{

}
