#pragma once

#include "TemportalEnginePCH.hpp"

#include "types/integer.h"

#include <array>
#include <optional>
#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class LogicalDevice;
class PhysicalDevice;

class Buffer
{

public:

	Buffer& setSize(ui64 size);

	void create(LogicalDevice const *pDevice);
	void destroy();

	void* get();

	template <typename T, ui32 Size>
	void write(std::array<T, Size> const &src)
	{
		this->write_internal((void*)src.data(), sizeof(T) * (ui32)src.size());
	}

private:
	ui64 mSize;

	LogicalDevice const *mpDevice;
	vk::UniqueBuffer mInternal;
	vk::UniqueDeviceMemory mBufferMemory;

	std::optional<ui32> findMemoryType(PhysicalDevice const *pDevice, ui32 typeFilter, vk::MemoryPropertyFlags propertyFlags);

	void write_internal(void* src, ui32 size);
};

NS_END
