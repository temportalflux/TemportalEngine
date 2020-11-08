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

	i32 categoryIndex;
	CategoryMeta* prevCategory;
	CategoryMeta* nextCategory;

	uIndex firstIndex() const;
	uIndex lastIndex() const;
	bool containsIndex(uIndex idx) const;

	void expandLeft();
	void shrinkLeft();
	void expandRight();
	void shrinkRight();

	void decrementStart();
	void incrementStart();
	void updateIndex();

};

class VoxelInstanceCategoryList
{

public:
	typedef std::pair<world::Coordinate, uIndex> CoordinateIndex;

	VoxelInstanceCategoryList(
		uSize totalValueCount, graphics::Buffer* buffer,
		std::unordered_set<game::BlockId> const& voxelIds
	);

	void clear();

	uSize unallocatedCount() const;

	uIndex allocate();
	void setCoordinateIndex(world::Coordinate const& coordinate, uIndex idx);
	void removeCoordinateIndex(world::Coordinate const& coordinate);

	std::optional<uIndex> searchForCoordinateIndex(world::Coordinate const& coordinate) const;

	CategoryMeta& getCategoryForValueIndex(uIndex idx);
	CategoryMeta& getCategoryForId(std::optional<game::BlockId> id);
	CategoryMeta& getCategory(uIndex categoryIndex);

private:

	std::vector<CategoryMeta> mOrderedCategories;

	/**
	 * Any voxel instances which HAVE been allocated, but their type is empty (i.e. air).
	 */
	CategoryMeta mEmpty;

	/**
	 * Any memory for voxels which is just not allocated.
	 * This is different from `mEmptyVoxels` b/c this is how we know how much data we can yet fill.
	 * This relates to both `mInstanceData` (where the instances are stored)
	 * and `mAllocatedVoxelsByCoordinate` (which relates a specific coordinate to a value in `mInstanceData`).
	 */
	CategoryMeta mUnallocated;

	std::unordered_map<game::BlockId, CategoryMeta*> mCategoryById;

	/**
	 * The index of a given instance in `mInstanceData` by world coordinate.
	 * Mutated by `changeVoxelId()`.
	 */
	// TODO: This is basically the world-state and should eventually be moved to reflect such
	typedef std::vector<CoordinateIndex> CoordinateToIndexList;
	typedef std::vector<std::optional<world::Coordinate>> CoordinateList;
	CoordinateToIndexList mCoordinateToIndex;
	CoordinateList mIndexToCoordinate; // TODO: this doesnt need to be a vector, it can be raw data

	std::optional<CoordinateToIndexList::const_iterator> searchForCoordinateIterator(world::Coordinate const& coordinate) const;

};

class BlockInstanceBuffer
{

public:

	static graphics::AttributeBinding getBinding(ui8& slot);

	/**
	 * Initializes the non-committed data with some amount of uncategorized values.
	 * Sets the size of the graphics buffer based on the value of `totalValueCount`.
	 */
	BlockInstanceBuffer(uSize totalValueCount, std::unordered_set<game::BlockId> const& voxelIds);
	~BlockInstanceBuffer();

	uSize size() const;

	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device);
	void createBuffer();

	void allocateCoordinates(std::vector<world::Coordinate> const& coordinates);

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

	/**
	 * The category metadata about `mInstanceData`.
	 * This is mutated by `changeVoxelId()` and is not safe to read from when rendering information from `mInstanceBuffer`.
	 */
	VoxelInstanceCategoryList mMutableCategoryList;
	/**
	 * The category metadata about `mInstanceData` currently stored in `mInstanceBuffer`.
	 * This is mutated by `commitToBuffer()` and is safe to read from when rendering information from `mInstanceBuffer`.
	 */
	std::unordered_map<game::BlockId, CategoryMeta> mCommittedCategories;

	/**
	 * Indices of `mInstanceData` which have changed since the last commit.
	 * Mutated by `commitToBuffer()`. Used to ensure the minimum number of writes to the buffer as possible per call.
	 */
	std::unordered_set<uIndex> mChangedBufferIndices;

	/**
	 * The literal GPU buffer which is written to via `commitToBuffer()`.
	 */
	graphics::Buffer mInstanceBuffer;

	ValueData* getInstanceAt(uIndex idx);

	/**
	 * Copies data in `mInstanceData` from the index `src` to `dest`,
	 * marking the `dest` index in `mChangedBufferIndices` in the process
	 */
	void copyInstanceData(uIndex const& src, uIndex const& dest);
	void setInstanceData(uIndex const& idx, ValueData const *const data);

};

NS_END
