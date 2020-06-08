#include "graphics/LogicalDevice.hpp"

#include "graphics/PhysicalDevice.hpp"

using namespace graphics;

LogicalDevice::LogicalDevice(PhysicalDevice const *pPhysicalDevice, vk::UniqueDevice &device)
{
	mpPhysicalDevice = pPhysicalDevice;
	mDevice.swap(device);
}

void* LogicalDevice::get()
{
	return &this->mDevice.get();
}

bool LogicalDevice::isValid() const
{
	// Checks underlying structure for VK_NULL_HANDLE
	return (bool)this->mDevice;
}

void LogicalDevice::invalidate()
{
	this->mpPhysicalDevice = nullptr;
	this->mDevice.reset();
}

std::unordered_map<QueueFamily::Enum, vk::Queue> LogicalDevice::findQueues(std::set<QueueFamily::Enum> types) const
{
	auto queues = std::unordered_map<QueueFamily::Enum, vk::Queue>();

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

void LogicalDevice::waitUntilIdle() const
{
	this->mDevice->waitIdle();
}
