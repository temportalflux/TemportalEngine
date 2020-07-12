#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class GraphicsDevice;
class Image;
class Buffer;

class Memory
{
	friend class GraphicsDevice;

	struct Slot
	{
		ui64 offset;
		ui64 size;
	};

public:
	Memory() = default;
	~Memory();

	Memory& setFlags(vk::MemoryPropertyFlags flags);
	Memory& configureSlot(vk::MemoryRequirements const &requirements, uIndex &outSlotIndex);

	Memory& create(std::shared_ptr<GraphicsDevice> device);
	void destroy();

	Memory& bind(std::shared_ptr<GraphicsDevice> device, uIndex const idxSlot, Buffer const *buffer);
	Memory& bind(std::shared_ptr<GraphicsDevice> device, uIndex const idxSlot, Image const *image);

	template <typename TData>
	void write(std::shared_ptr<GraphicsDevice> device, uIndex const idxSlot, std::vector<TData> data)
	{
		this->write(device, this->mSlots[idxSlot].offset, data.data(), data.size() * sizeof(TData));
	}

private:
	vk::MemoryPropertyFlags mFlags;
	std::optional<ui32> mMemoryTypeBits;
	// List of entries created by calling `appendRequirements`.
	std::vector<Slot> mSlots;
	ui64 mTotalSize;

	vk::UniqueDeviceMemory mInternal;

	void write(std::shared_ptr<GraphicsDevice> device, ui64 offset, void* src, uSize size);

};

NS_END
