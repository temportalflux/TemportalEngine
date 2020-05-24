#include "graphics/LogicalDevice.hpp"

#include "graphics/PhysicalDevice.hpp"

using namespace graphics;

LogicalDevice::LogicalDevice(PhysicalDevice const *pPhysicalDevice, vk::UniqueDevice &device)
{
	mpPhysicalDevice = pPhysicalDevice;
	mDevice.swap(device);
}

std::unordered_map<QueueFamily, vk::Queue> LogicalDevice::findQueues(std::set<QueueFamily> types) const
{
	auto queues = std::unordered_map<QueueFamily, vk::Queue>();

	if (mpPhysicalDevice == nullptr) return queues;
	auto queueGroup = this->mpPhysicalDevice->queryQueueFamilyGroup();

	for (auto& queueFamilyType : types)
	{
		auto idxQueueFamily = queueGroup.getQueueIndex(queueFamilyType);
		if (idxQueueFamily.has_value())
		{
			queues.insert(std::make_pair(
				queueFamilyType,
				mDevice->getQueue(idxQueueFamily.value(), /*subqueue index*/ 0)
			));
		}
	}

	return queues;
}

void LogicalDevice::invalidate()
{
	this->mpPhysicalDevice = nullptr;
	this->mDevice.reset();
}
