#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/Command.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS

class CommandBuffer
{
	friend class CommandPool;
	friend class Command;

public:
	CommandBuffer() = default;

	Command beginCommand();

private:
	vk::UniqueCommandBuffer mInternal;

	void endCommand(Command *command);

};

NS_END
