#pragma once

#include "CoreInclude.hpp"

#include "ecs/types.h"
#include "graphics/Buffer.hpp"
#include "math/Matrix.hpp"
#include "FixedSortedArray.hpp"
#include "thread/MutexLock.hpp"
#include "utility/DynamicHandle.hpp"

NS_GRAPHICS
class GraphicsDevice;
class CommandPool;

struct EntityInstanceData
{
	math::Vector3Padded posOfCurrentChunk;
	math::Matrix4x4 localTransform;
};

class EntityInstanceBuffer : public IDynamicHandleOwner<EntityInstanceData>
{

	struct InstanceMeta
	{
		bool bIsActive;
		bool bHasChanged;
	};

public:

	EntityInstanceBuffer();
	~EntityInstanceBuffer();

	void setDevice(std::weak_ptr<GraphicsDevice> device);
	void create();

	bool hasChanges() const;
	void commitToBuffer(graphics::CommandPool* transientPool);

	graphics::Buffer* buffer();

public:
	DynamicHandle<EntityInstanceData> createHandle() override;
	EntityInstanceData* get(uIndex const& idx) override;
	void destroyHandle(uIndex const& idx) override;
	void markDirty(uIndex const& idx);

private:
	static constexpr ui32 instanceBufferCount() { return ECS_MAX_ENTITY_COUNT; }
	static constexpr uSize instanceBufferSize() { return sizeof(EntityInstanceData) * instanceBufferCount(); }
	static constexpr ui32 stagingBufferItemCount() { return math::min(ui32(16), instanceBufferCount()); }
	static constexpr uSize stagingBufferSize() { return sizeof(EntityInstanceData) * stagingBufferItemCount(); }

	thread::MutexLock mMutex;

	std::array<EntityInstanceData, ECS_MAX_ENTITY_COUNT> mInstances;
	FixedSortedArray<uIndex, ECS_MAX_ENTITY_COUNT> mUnusedInstanceIndices;
	std::array<InstanceMeta, ECS_MAX_ENTITY_COUNT> mInstanceMetadata;
	bool bHasAnyChanges;

	graphics::Buffer mStagingBuffer;
	graphics::Buffer mInstanceBuffer;

};

NS_END
