#pragma once

#include "graphics/DeviceObject.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class GraphicsDevice;
class Image;
class Buffer;

class Memory : public DeviceObject
{
	friend class GraphicsDevice;

	struct Slot
	{
		ui64 offset;
		ui64 size;
	};

public:
	Memory() : mTotalSize(0) {}
	~Memory();

	Memory& setFlags(vk::MemoryPropertyFlags flags);
	Memory& configureSlot(vk::MemoryRequirements const &requirements, uIndex &outSlotIndex);

	void create() override;
	void* get() override;
	void invalidate() override;
	void resetConfiguration() override;

	Memory& bind(uIndex const idxSlot, Buffer const *buffer);
	Memory& bind(uIndex const idxSlot, Image const *image);

	template <typename TData>
	void write(uIndex const idxSlot, std::vector<TData> data, bool bClear=false)
	{
		this->write(idxSlot, 0, data.data(), data.size() * sizeof(TData), bClear);
	}
	void write(ui64 const idxSlot, uSize const offset, void* src, uSize size, bool bClear=false)
	{
		auto const& slot = this->mSlots[idxSlot];
		this->writeInternal(slot.offset, slot.size, offset, src, size, bClear);
	}

private:
	vk::MemoryPropertyFlags mFlags;
	std::optional<ui32> mMemoryTypeBits;
	// List of entries created by calling `appendRequirements`.
	std::vector<Slot> mSlots;
	ui64 mTotalSize;

	vk::UniqueDeviceMemory mInternal;

	void writeInternal(ui64 const mapOffset, ui64 const mapSize, uSize const dataOffset, void* src, uSize size, bool bClear);

};

NS_END
