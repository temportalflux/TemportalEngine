#include "graphics/LogicalDeviceInfo.hpp"

#include "types/integer.h"
#include "graphics/QueueFamilyGroup.hpp"

using namespace graphics;

LogicalDeviceInfo& LogicalDeviceInfo::addQueueFamily(QueueFamily type)
{
	this->mQueues.push_back(type);
	return *this;
}

LogicalDeviceInfo& LogicalDeviceInfo::addDeviceExtension(char const* name)
{
	this->mDeviceExtensions.push_back(name);
	return *this;
}

LogicalDeviceInfo& LogicalDeviceInfo::setValidationLayers(std::vector<char const*> layers)
{
	this->mValidationLayers = layers;
	return *this;
}

vk::DeviceQueueCreateInfo QueueInfo::makeInfo() const
{
	return vk::DeviceQueueCreateInfo()
		.setQueueFamilyIndex(idxQueueFamily)
		.setQueueCount((ui32)queuePriorities.size())
		.setPQueuePriorities(queuePriorities.data());
}

std::vector<QueueInfo> LogicalDeviceInfo::makeQueueInfo(QueueFamilyGroup const *pAvailableQueues) const
{
	auto queueCount = this->mQueues.size();
	auto queueCreateInfo = std::vector<QueueInfo>(queueCount);
	for (uSize i = 0; i < queueCount; ++i)
	{
		auto idxQueueFamily = pAvailableQueues->getQueueIndex(this->mQueues[i]);
		assert(idxQueueFamily.has_value());

		queueCreateInfo[i].idxQueueFamily = idxQueueFamily.value();
		// TODO: allow each queue to specify their sub-queue priorities
		queueCreateInfo[i].queuePriorities.push_back(1.0f);
	}
	return queueCreateInfo;
}
