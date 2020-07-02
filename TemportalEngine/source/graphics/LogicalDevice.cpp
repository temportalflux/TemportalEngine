#include "graphics/LogicalDevice.hpp"

#include "graphics/PhysicalDevice.hpp"

using namespace graphics;

LogicalDevice::LogicalDevice(PhysicalDevice const *pPhysicalDevice, vk::UniqueDevice &device)
{
	mpPhysicalDevice = pPhysicalDevice;
	this->mInternal.swap(device);
}

void* LogicalDevice::get()
{
	return &this->mInternal.get();
}

bool LogicalDevice::isValid() const
{
	// Checks underlying structure for VK_NULL_HANDLE
	return (bool)this->mInternal;
}

void LogicalDevice::invalidate()
{
	this->mpPhysicalDevice = nullptr;
	this->mInternal.reset();
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
				this->mInternal->getQueue(idxQueueFamily.value(), /*subqueue index*/ 0)
			));
		}
	}

	return queues;
}

void LogicalDevice::waitUntilIdle() const
{
	this->mInternal->waitIdle();
}

void LogicalDevice::waitFor(std::vector<vk::Fence> fences, bool bAll, ui64 timeout)
{
	this->mInternal->waitForFences(fences, bAll, timeout);
}
