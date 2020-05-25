#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS

class CommandBuffer
{
	friend class CommandPool;

public:
	CommandBuffer() = default;

private:
	vk::UniqueCommandBuffer mInternal;

};

NS_END
