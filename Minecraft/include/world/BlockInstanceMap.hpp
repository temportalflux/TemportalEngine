#pragma once

#include "CoreInclude.hpp"

#include "math/Matrix.hpp"
#include "graphics/Buffer.hpp"
#include "world/BlockMetadata.hpp"
#include "world/WorldCoordinate.hpp"
#include "graphics/AttributeBinding.hpp"

FORWARD_DEF(NS_GRAPHICS, class Command)
FORWARD_DEF(NS_GRAPHICS, class GraphicsDevice)
FORWARD_DEF(NS_GRAPHICS, class Memory)

NS_WORLD

class BlockInstanceBuffer
{

public:

	struct CategoryMeta
	{

		/**
		 * A pointer to the underlying shared-buffer for all category values.
		 */
		graphics::Buffer* buffer;

		/**
		 * The index at which values start in `mInstanceData` (and `buffer`) for a given category.
		 */
		uIndex index;

		/**
		 * The number of values in `mInstanceData` (and `buffer`) for a given category.
		 */
		uSize count;

	};

	/**
	 * Initializes the non-committed data with some amount of uncategorized values.
	 * Sets the size of the graphics buffer based on the value of `totalValueCount`.
	 */
	BlockInstanceBuffer(uSize totalValueCount, std::unordered_set<game::BlockId> const& voxelIds);
	~BlockInstanceBuffer();

	uSize size() const;

	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device);
	void createBuffer();

	/**
	 * Changes an instance at `coordinate` from its current category to the `desiredVoxelId`.
	 * Changes are prepared in CPU data until `commitToBuffer` is called.
	 */
	void changeVoxelId(world::Coordinate const& coordinate, std::optional<game::BlockId> const desiredVoxelId);
	
	bool hasChanges() const;

	/**
	 * Writes any changes made by `changeVoxelId` to the GPU buffer object.
	 */
	void commitToBuffer();

	CategoryMeta const& getDataForVoxelId(game::BlockId const& id) const;

private:

	struct ValueData
	{
		/**
		 * the coordinate of the chunk this block is in
		 */
		math::Vector3Padded posOfChunk;
		/**
		 * the position of the block in the chunk, as well as any rotation on its local origin
		 */
		math::Matrix4x4 model;
	};

	uSize mTotalInstanceCount;
	ValueData* mInstanceData;

	/**
	 * The memory for `mInstanceBuffer`.
	 */
	std::shared_ptr<graphics::Memory> mpMemory;

	CategoryMeta mUnallocatedVoxels;

	/**
	 * The category metadata about `mInstanceData`.
	 * This is mutated by `changeVoxelId()` and is not safe to read from when rendering information from `mInstanceBuffer`.
	 */
	std::unordered_map<game::BlockId, CategoryMeta> mMutableCategories;

	/**
	 * The category metadata about `mInstanceData` currently stored in `mInstanceBuffer`.
	 * This is mutated by `commitToBuffer()` and is safe to read from when rendering information from `mInstanceBuffer`.
	 */
	std::unordered_map<game::BlockId, CategoryMeta> mCommittedCategories;

	/**
	 * The index of a given instance in `mInstanceData` by world coordinate.
	 * Mutated by `changeVoxelId()`.
	 */
	// TODO: Coordinates are not hashable (infinite space). Need to create a sorted list of coordinates->valueIdx which can be binary searched.
	//std::unordered_map<world::Coordinate, uIndex> mValueIndices;

	/**
	 * Indices of `mInstanceData` which have changed since the last commit.
	 * Mutated by `commitToBuffer()`. Used to ensure the minimum number of writes to the buffer as possible per call.
	 */
	std::unordered_set<uIndex> mChangedBufferIndicies;

	/**
	 * The literal GPU buffer which is written to via `commitToBuffer()`.
	 */
	graphics::Buffer mInstanceBuffer;

};

class BlockInstanceMap
{
	typedef std::function<void(
		world::Coordinate const&, std::optional<BlockMetadata> const&, std::optional<BlockMetadata> const&
	)> TOnBlockChanged;

public:

	struct RenderData
	{
		math::Vector3Padded posOfChunk; // the coordinate of the chunk this block is in
		math::Matrix4x4 model; // the position of the block in the chunk, as well as any rotation on its local origin
	};

	struct InstanceData
	{
		graphics::Buffer* buffer;
		ui32 offset;
		ui32 count;
	};

	static graphics::AttributeBinding getBinding(ui8& slot);

	BlockInstanceMap();
	~BlockInstanceMap();

	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device);

	/**
	 * Recreates the instance buffer for the desired size.
	 * Assumes the instance buffer is not currently in use (rendering should be disabled when this function is called).
	 */
	void constructInstanceBuffer(ui8 chunkRenderDistance);

	void invalidate();

	TOnBlockChanged onBlockChangedListener();

	void updateCoordinate(
		world::Coordinate const& global,
		std::optional<BlockMetadata> const& prev,
		std::optional<BlockMetadata> const& next
	);

	void writeInstanceBuffer(graphics::CommandPool* transientPool);

	InstanceData getBlockInstanceData(game::BlockId const &id);

private:
	std::shared_ptr<graphics::Memory> mpInstanceMemory;
	graphics::Buffer mInstanceBuffer;
	ui32 mInstanceCount;

	ui64 getInstanceBufferSize(ui8 chunkRenderDistance) const;

};

NS_END
