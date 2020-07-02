#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/types.hpp"
#include "graphics/QueueFamilyGroup.hpp"
#include "graphics/CommandBuffer.hpp"
#include "types/integer.h"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;

class CommandPool
{

public:
	CommandPool() = default;

	CommandPool& setQueueFamily(QueueFamily::Enum queueType, QueueFamilyGroup const &group);

	bool isValid() const;
	CommandPool& create(LogicalDevice *pDevice, vk::CommandPoolCreateFlags flags = vk::CommandPoolCreateFlags());
	void destroy();

	std::vector<CommandBuffer> createCommandBuffers(uSize count) const;
	void resetPool();

private:
	QueueFamily::Enum mQueueFamily;
	ui32 mIdxQueueFamily;

	LogicalDevice *mpDevice;

	vk::UniqueCommandPool mInternal;

};

NS_END
