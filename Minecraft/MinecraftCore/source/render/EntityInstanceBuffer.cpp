#include "render/EntityInstanceBuffer.hpp"

#include "graphics/GraphicsDevice.hpp"
#include "graphics/CommandPool.hpp"

using namespace graphics;

EntityInstanceBuffer::EntityInstanceBuffer() : mMutex()
{
	this->mStagingBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferSrc, graphics::MemoryUsage::eCPUSource)
		.setSize(EntityInstanceBuffer::stagingBufferSize());
	this->mInstanceBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, graphics::MemoryUsage::eGPUOnly)
		.setSize(EntityInstanceBuffer::instanceBufferSize());
	
	this->bHasAnyChanges = false;
	for (uIndex idx = 0; idx < EntityInstanceBuffer::instanceBufferCount(); ++idx)
	{
		this->mUnusedInstanceIndices.push(idx);
		this->mInstances[idx] = EntityInstanceData{};
		this->mInstanceMetadata[idx] = InstanceMeta { false, false };
	}
}

EntityInstanceBuffer::~EntityInstanceBuffer()
{
	this->mStagingBuffer.destroy();
	this->mInstanceBuffer.destroy();
}

void EntityInstanceBuffer::setDevice(std::weak_ptr<GraphicsDevice> device)
{
	this->mStagingBuffer.setDevice(device);
	this->mInstanceBuffer.setDevice(device);
}

void EntityInstanceBuffer::create()
{
	OPTICK_EVENT();
	this->mStagingBuffer.create();
	this->mInstanceBuffer.create();
}

DynamicHandle<EntityInstanceData> EntityInstanceBuffer::createHandle()
{
	this->mMutex.lock();
	auto idx = this->mUnusedInstanceIndices.dequeue();
	this->mInstanceMetadata[idx].bIsActive = true;
	this->bHasAnyChanges = true;
	this->mMutex.unlock();
	return DynamicHandle<EntityInstanceData>(this->weak_from_this(), idx);
}

EntityInstanceData* EntityInstanceBuffer::get(uIndex const& idx)
{
	return &this->mInstances[idx];
}

void EntityInstanceBuffer::destroyHandle(uIndex const& idx)
{
	this->mMutex.lock();
	this->mUnusedInstanceIndices.insert(idx);
	this->mInstanceMetadata[idx].bIsActive = false;
	this->bHasAnyChanges = true;
	this->mMutex.unlock();
}

void EntityInstanceBuffer::setData(DynamicHandle<EntityInstanceData> const& handle, EntityInstanceData const& data)
{
	this->mMutex.lock();
	this->mInstances[uIndex(handle)] = data;
	this->mInstanceMetadata[uIndex(handle)].bHasChanged = true;
	this->bHasAnyChanges = true;
	this->mMutex.unlock();
}

bool EntityInstanceBuffer::hasChanges() const
{
	return this->bHasAnyChanges;
}

void EntityInstanceBuffer::commitToBuffer(graphics::CommandPool* transientPool)
{
	OPTICK_EVENT();
	assert(this->bHasAnyChanges);

	// Copy buffer data while locked so the overarching buffer is locked for less time
	this->mMutex.lock();
	auto instances = this->mInstances;
	auto metadata = this->mInstanceMetadata;
	this->bHasAnyChanges = false;
	for (uIndex idx = 0; idx < EntityInstanceBuffer::instanceBufferCount(); ++idx) this->mInstanceMetadata[idx].bHasChanged = false;
	this->mMutex.unlock();

	auto* pStaging = &this->mStagingBuffer;
	auto* pGPUBuffer = &this->mInstanceBuffer;

	auto regionsToCopy = std::vector<graphics::BufferRegionCopy>();
	auto commitRegions = [pStaging, pGPUBuffer, &regionsToCopy, transientPool]()
	{
		if (regionsToCopy.size() > 0)
		{
			transientPool->submitOneOff([pStaging, pGPUBuffer, &regionsToCopy](graphics::Command* cmd) {
				cmd->copyBuffer(pStaging, pGPUBuffer, regionsToCopy);
			});
			regionsToCopy.clear();
		}
	};

	auto regionBeingPrepared = graphics::BufferRegionCopy{ 0, 0, 0 };
	uSize totalStagingBufferSizeWritten = 0;
	std::optional<uIndex> prevIdx = std::nullopt;
	uIndex nextIdx = 0;
	while (nextIdx < EntityInstanceBuffer::instanceBufferCount())
	{
		if (metadata[nextIdx].bIsActive && metadata[nextIdx].bHasChanged)
		{
			// If the previous entry and next entry are contiguous (next to each other in the buffer)
			// the the current region can be expanded instead of adding a new region
			if (prevIdx.has_value() && nextIdx == *prevIdx + 1) regionBeingPrepared.size += sizeof(EntityInstanceData);
			// The entries are not contiguous, so we must append the previous region and start a new one
			else
			{
				if (regionBeingPrepared.size > 0)
				{
					regionsToCopy.push_back(regionBeingPrepared);
					if (totalStagingBufferSizeWritten == EntityInstanceBuffer::stagingBufferSize())
					{
						commitRegions();
					}
				}
				regionBeingPrepared = {
					/*srcOffset*/ totalStagingBufferSizeWritten,
					/*dstOffset*/ nextIdx * sizeof(EntityInstanceData),
					sizeof(EntityInstanceData)
				};
			}
			this->mStagingBuffer.write(
				totalStagingBufferSizeWritten,
				&instances[nextIdx], sizeof(EntityInstanceData), false
			);
			totalStagingBufferSizeWritten += sizeof(EntityInstanceData);
		}
		prevIdx = nextIdx;
		nextIdx++;
	}
	if (regionBeingPrepared.size > 0) regionsToCopy.push_back(regionBeingPrepared);
	commitRegions();
}

graphics::Buffer* EntityInstanceBuffer::buffer()
{
	return &this->mInstanceBuffer;
}
