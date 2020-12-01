#pragma once

#include "graphics/DeviceObject.hpp"

#include "graphics/Command.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS

class CommandBuffer : public DeviceObject
{
	friend class GraphicsDevice;
	friend class Command;
	friend class Frame;

public:
	CommandBuffer() = default;
	CommandBuffer(CommandBuffer &&other);
	CommandBuffer& operator=(CommandBuffer &&other);
	~CommandBuffer();

	void* get() override;
	void invalidate() override;

	Command beginCommand(vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlags());

private:
	vk::UniqueCommandBuffer mInternal;

	void endCommand(Command *command);

};

NS_END
