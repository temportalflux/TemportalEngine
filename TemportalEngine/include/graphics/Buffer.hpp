#pragma once

#include "graphics/DeviceObject.hpp"
#include "graphics/MemoryAllocated.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class CommandPool;

class Buffer : public DeviceObject, public MemoryAllocated
{
	friend class GraphicsDevice;

public:
	Buffer() : mSize(0) {}

	Buffer& setUsage(vk::BufferUsageFlags flags);
	Buffer& setSize(uSize size);
	uSize getSize() const;

	void create() override;
	void* get() override;
	void invalidate() override;
	void resetConfiguration() override;

	void bindMemory() override;

	template <typename TData>
	void writeBuffer(CommandPool* transientPool, uSize offset, std::vector<TData> const &dataSet)
	{
		this->writeBuffer(transientPool, offset, (void*)dataSet.data(), sizeof(TData) * (uSize)dataSet.size());
	}
	void writeBuffer(CommandPool* transientPool, uSize offset, void* data, uSize size, bool bClear=false);

	void write(uSize const offset, void* src, uSize size, bool bClear = false);

	static void writeDataToGPU(
		std::shared_ptr<GraphicsDevice> device,
		CommandPool* transientPool,
		uSize const bufferSize, bool const bClear,
		uSize const dataOffset, void* data, uSize const dataSize,
		std::function<void(class Command* cmd, Buffer* stagingBuffer)> writeCommands
	);

private:
	vk::BufferUsageFlags mUsageFlags;
	uSize mSize;
	vk::UniqueBuffer mInternal;

	vk::MemoryRequirements getRequirements() const override;

};

NS_END
