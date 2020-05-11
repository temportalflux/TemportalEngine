#include "render/Renderer.hpp"
#include "Engine.hpp"
#include "ExecutableInfo.hpp"

using namespace render;
//using namespace vk;

Renderer::Renderer(
	utility::SExecutableInfo const *const appInfo,
	utility::SExecutableInfo const *const engineInfo
)
	: maRequiredExtensionNames({
		"VK_KHR_surface",
		"VK_KHR_win32_surface",
	})
{

	mpApplicationInfo->pApplicationName = appInfo->title;
	mpApplicationInfo->applicationVersion = appInfo->version;
	mpApplicationInfo->pEngineName = engineInfo->title;
	mpApplicationInfo->engineVersion = engineInfo->version;
	mpApplicationInfo->apiVersion = VK_API_VERSION_1_1;

	mpInstanceInfo->pApplicationInfo = mpApplicationInfo;
	mpInstanceInfo->setPpEnabledExtensionNames(maRequiredExtensionNames.data());

	{
		auto appInstance = memory::NewUnique<vk::UniqueInstance>();
		assert(appInstance.has_value());
		mpAppInstance = appInstance.value();
	}
	*(mpAppInstance.GetRaw()) = vk::createInstanceUnique(*mpInstanceInfo);

	auto layerProps = vk::enumerateInstanceLayerProperties();
	auto physicalDevices = mpAppInstance->get().enumeratePhysicalDevices();
	
	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevices[0].getQueueFamilyProperties();

	// Find the index of a queue family which supports graphics
	size_t graphicsQueueFamilyIndex = std::distance(
		queueFamilyProperties.begin(),
		// Find the iterator for a family property which supports graphics
		std::find_if(
			queueFamilyProperties.begin(),
			queueFamilyProperties.end(),
			[](vk::QueueFamilyProperties const &familyProps) {
				// Does the iterator have the graphics flag set
				return familyProps.queueFlags & vk::QueueFlagBits::eGraphics;
			}
		)
	);
	// Ensure that there is at least 1
	assert(graphicsQueueFamilyIndex < queueFamilyProperties.size());

	float queuePriority = 0.0f;
	vk::DeviceQueueCreateInfo deviceQueueCreateInfo(
		vk::DeviceQueueCreateFlags(),
		static_cast<ui32>(graphicsQueueFamilyIndex),
		1,
		&queuePriority
	);
	vk::UniqueDevice device = physicalDevices[0].createDeviceUnique(vk::DeviceCreateInfo(vk::DeviceCreateFlags(), 1, &deviceQueueCreateInfo));

	vk::UniqueCommandPool commandPool = device->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(), deviceQueueCreateInfo.queueFamilyIndex));

	std::vector<vk::UniqueCommandBuffer> commandBuffers = device->allocateCommandBuffersUnique(
		vk::CommandBufferAllocateInfo(commandPool.get(), vk::CommandBufferLevel::ePrimary, 1)
	);

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
