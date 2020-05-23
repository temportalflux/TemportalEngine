#include "graphics/LogicalDevice.hpp"

#include "graphics/PhysicalDevice.hpp"

using namespace graphics;

LogicalDevice::LogicalDevice(PhysicalDevice const *pPhysicalDevice, vk::UniqueDevice &device)
{
	mpPhysicalDevice = pPhysicalDevice;
	mDevice.swap(device);
}

std::optional<vk::Queue> LogicalDevice::getQueue(QueueFamily type) const
{
	if (mpPhysicalDevice == nullptr) return std::nullopt;
	auto queueGroup = this->mpPhysicalDevice->queryQueueFamilyGroup();
	auto idxQueueFamily = queueGroup.getQueueIndex(type);
	if (!idxQueueFamily.has_value()) return std::nullopt;
	return mDevice->getQueue(idxQueueFamily.value(), /*subqueue index*/ 0);
}

void LogicalDevice::invalidate()
{
	this->mpPhysicalDevice = nullptr;
	this->mDevice.reset();
}
