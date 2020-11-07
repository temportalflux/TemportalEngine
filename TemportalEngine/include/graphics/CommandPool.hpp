#pragma once

#include "graphics/DeviceObject.hpp"

#include "graphics/types.hpp"
#include "graphics/QueueFamilyGroup.hpp"
#include "graphics/CommandBuffer.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS

class CommandPool : public DeviceObject
{
	friend class GraphicsDevice;

public:
	CommandPool();
	CommandPool(CommandPool &&other);
	CommandPool& operator=(CommandPool &&other);
	~CommandPool();

	CommandPool& setFlags(vk::CommandPoolCreateFlags flags);
	CommandPool& setQueueFamily(QueueFamily::Enum queueType, QueueFamilyGroup const &group);

	bool isValid() const;
	void create() override;
	void* get() override;
	void invalidate() override;
	void resetConfiguration() override;

	std::vector<CommandBuffer> createCommandBuffers(ui32 const count) const;
	void resetPool();

	void submitOneOff(std::function<void(class Command* cmd)> writeCommands);

private:
	vk::CommandPoolCreateFlags mCreateFlags;
	std::optional<QueueFamily::Enum> mQueueFamily;
	std::optional<ui32> mIdxQueueFamily;

	vk::UniqueCommandPool mInternal;

};

NS_END
