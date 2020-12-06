#pragma once

#include "CoreInclude.hpp"

#include "ecs/types.h"
#include "graphics/Buffer.hpp"
#include "math/Matrix.hpp"
#include "FixedSortedArray.hpp"
#include "thread/MutexLock.hpp"

NS_GRAPHICS
class GraphicsDevice;
class CommandPool;

class EntityInstanceBuffer
{

	struct InstanceMeta
	{
		bool bIsActive;
		bool bHasChanged;
	};

public:

	struct InstanceData
	{
		math::Vector3Padded posOfCurrentChunk;
		math::Matrix4x4 localTransform;
	};

	EntityInstanceBuffer();
	~EntityInstanceBuffer();

	void setDevice(std::weak_ptr<GraphicsDevice> device);
	void create();

	uIndex createInstance();
	void destroyInstance(uIndex const& handle);
	void markInstanceForUpdate(uIndex const& handle, InstanceData const& data);

	bool hasChanges() const;
	void commitToBuffer(graphics::CommandPool* transientPool);

	graphics::Buffer* buffer();

private:
	static constexpr ui32 instanceBufferCount() { return ECS_MAX_ENTITY_COUNT; }
	static constexpr uSize instanceBufferSize() { return sizeof(InstanceData) * instanceBufferCount(); }
	static constexpr ui32 stagingBufferItemCount() { return math::min(ui32(16), instanceBufferCount()); }
	static constexpr uSize stagingBufferSize() { return sizeof(InstanceData) * stagingBufferItemCount(); }

	thread::MutexLock mMutex;

	std::array<InstanceData, ECS_MAX_ENTITY_COUNT> mInstances;
	FixedSortedArray<uIndex, ECS_MAX_ENTITY_COUNT> mUnusedInstanceIndices;
	std::array<InstanceMeta, ECS_MAX_ENTITY_COUNT> mInstanceMetadata;
	bool bHasAnyChanges;

	graphics::Buffer mStagingBuffer;
	graphics::Buffer mInstanceBuffer;

};

NS_END
