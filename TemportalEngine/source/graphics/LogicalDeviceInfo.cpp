#include "graphics/LogicalDeviceInfo.hpp"

#include "types/integer.h"
#include "graphics/QueueFamilyGroup.hpp"

#include <unordered_set>

using namespace graphics;

LogicalDeviceInfo& LogicalDeviceInfo::addQueueFamily(QueueFamily::Enum type)
{
	this->mQueues.push_back(type);
	return *this;
}

std::set<QueueFamily::Enum> LogicalDeviceInfo::getQueues() const
{
	return std::set<QueueFamily::Enum>(this->mQueues.begin(), this->mQueues.end());
}

LogicalDeviceInfo& LogicalDeviceInfo::addDeviceExtension(std::string name)
{
	this->mDeviceExtensions.push_back(name);
	return *this;
}

LogicalDeviceInfo& LogicalDeviceInfo::setValidationLayers(std::vector<std::string> layers)
{
	this->mValidationLayers = layers;
	return *this;
}

std::vector<std::string> LogicalDeviceInfo::getValidationLayers() const
{
	return this->mValidationLayers;
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
	auto queueIndicies = std::unordered_set<ui32>();
	auto queueCreateInfo = std::vector<QueueInfo>();
	for (uSize i = 0; i < queueCount; ++i)
	{
		auto idxQueueFamily = pAvailableQueues->getQueueIndex(this->mQueues[i]);
		assert(idxQueueFamily.has_value());

		// If the queue family is not already in the info list
		if (queueIndicies.find(idxQueueFamily.value()) == queueIndicies.end())
		{
			queueIndicies.insert(idxQueueFamily.value());
			
			auto info = QueueInfo();
			info.idxQueueFamily = idxQueueFamily.value();
			// TODO: allow each queue to specify their sub-queue priorities
			info.queuePriorities.push_back(1.0f);

			queueCreateInfo.push_back(info);
		}
	}
	return queueCreateInfo;
}
