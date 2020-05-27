#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/Command.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS

class CommandBuffer
{
	friend class CommandPool;
	friend class Command;
	friend class Frame;

public:
	CommandBuffer() = default;
	CommandBuffer(CommandBuffer &&other);
	CommandBuffer& operator=(CommandBuffer &&other);

	void* get();

	Command beginCommand(vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlags());

private:
	vk::UniqueCommandBuffer mInternal;

	void endCommand(Command *command);

};

NS_END
