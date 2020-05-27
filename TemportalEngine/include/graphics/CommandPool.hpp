#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/QueueFamily.hpp"
#include "graphics/QueueFamilyGroup.hpp"
#include "graphics/CommandBuffer.hpp"
#include "types/integer.h"

#include <vector>
#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;

class CommandPool
{

public:
	CommandPool() = default;

	CommandPool& setQueueFamily(QueueFamily queueType, QueueFamilyGroup const &group);

	bool isValid() const;
	CommandPool& create(LogicalDevice const *pDevice, vk::CommandPoolCreateFlags flags = vk::CommandPoolCreateFlags());
	void destroy();

	std::vector<CommandBuffer> createCommandBuffers(uSize count) const;
	void resetPool();

private:
	QueueFamily mQueueFamily;
	ui32 mIdxQueueFamily;

	LogicalDevice const *mpDevice;

	vk::UniqueCommandPool mInternal;

};

NS_END
