#include "render/Renderer.hpp"
#include <vulkan/vulkan.hpp>

using namespace render;
using namespace vk;

Renderer::Renderer()
{
	const char* names[] = { "" };
	InstanceCreateInfo info = InstanceCreateInfo().setPpEnabledLayerNames(names);

	auto layerProps = enumerateInstanceLayerProperties();
	UniqueInstance instance = createInstanceUnique(info);
	auto physicalDevices = instance->enumeratePhysicalDevices();

	std::vector<QueueFamilyProperties> queueFamilyProps = physicalDevices[0].getQueueFamilyProperties();

	

}

Renderer::~Renderer()
{
}
