#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/types.hpp"
#include "graphics/QueueFamilyGroup.hpp"
#include "graphics/CommandBuffer.hpp"
#include "types/integer.h"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class GraphicsDevice;

class CommandPool
{
	friend class GraphicsDevice;

public:
	CommandPool() : mQueueFamily(QueueFamily::Enum::eGraphics), mIdxQueueFamily(0) {}

	CommandPool& setQueueFamily(QueueFamily::Enum queueType, QueueFamilyGroup const &group);

	bool isValid() const;
	CommandPool& create(std::shared_ptr<GraphicsDevice> device, vk::CommandPoolCreateFlags flags = vk::CommandPoolCreateFlags());
	void destroy();

	std::vector<CommandBuffer> createCommandBuffers(ui32 const count) const;
	void resetPool();

	void submitOneOff(std::function<void(class Command* cmd)> writeCommands);

private:
	QueueFamily::Enum mQueueFamily;
	ui32 mIdxQueueFamily;

	std::weak_ptr<GraphicsDevice> mpDevice;

	vk::UniqueCommandPool mInternal;

};

NS_END
