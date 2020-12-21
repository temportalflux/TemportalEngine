#include "render/voxel/BlockInstanceMap.hpp"

#include "graphics/Command.hpp"
#include "graphics/CommandPool.hpp"
#include "model/ModelVoxel.hpp"

using namespace world;

struct ValueCoordinateComparator
{
	bool operator() (VoxelInstanceCategoryList::CoordinateIndex const& src, world::Coordinate const& coord) const { return src.first < coord; }
	bool operator() (world::Coordinate const& coord, VoxelInstanceCategoryList::CoordinateIndex const& src) const { return coord < src.first; }
};

uIndex CategoryMeta::firstIndex() const
{
	return this->index;
}

uIndex CategoryMeta::lastIndex() const
{
	return this->firstIndex() + this->count - 1;
}

bool CategoryMeta::containsIndex(uIndex idx) const
{
	return this->count > 0 && this->firstIndex() <= idx && idx <= this->lastIndex();
}

void CategoryMeta::expandLeft()
{
	assert(this->index > 0);
	this->index--;
	this->count++;
	if (this->prevCategory != nullptr) this->prevCategory->shrinkLeft();
	assert(this->prevCategory == nullptr || this->prevCategory->lastIndex() + 1 == this->firstIndex());
	assert(this->nextCategory == nullptr || this->lastIndex() + 1 == this->nextCategory->firstIndex());
}

void CategoryMeta::shrinkLeft()
{
	assert(this->count > 0 || this->prevCategory != nullptr);
	if (this->count > 0)
	{
		this->count--;
	}
	else
	{
		if (this->prevCategory != nullptr)
		{
			this->prevCategory->shrinkLeft();
		}
		this->setIndexFromPrevCategory();
	}
}

void CategoryMeta::expandRight()
{
	assert(this->nextCategory != nullptr); // only unallocated category has a null next category
	this->count++;
	if (this->nextCategory != nullptr)
	{
		this->nextCategory->shrinkRight();
	}
	assert(this->prevCategory == nullptr || this->prevCategory->lastIndex() + 1 == this->firstIndex());
	assert(this->nextCategory == nullptr || this->lastIndex() + 1 == this->nextCategory->firstIndex());
}

void CategoryMeta::shrinkRight()
{
	assert(this->count > 0 || this->nextCategory != nullptr);
	if (this->count > 0)
	{
		this->index++;
		this->count--;
	}
	else
	{
		this->setIndexFromPrevCategory();
		if (this->nextCategory != nullptr)
		{
			this->nextCategory->shrinkRight();
		}
	}
}

void CategoryMeta::setIndexFromPrevCategory()
{
	assert(this->count == 0);
	this->index = this->prevCategory != nullptr ? this->prevCategory->index + this->prevCategory->count : 0;
}

VoxelInstanceCategoryList::VoxelInstanceCategoryList(
	uSize totalValueCount, graphics::Buffer* buffer,
	std::unordered_set<game::BlockId> const& voxelIds
)
{
	uSize const categoryCount = voxelIds.size();
	auto const categories = std::vector<game::BlockId>(voxelIds.begin(), voxelIds.end());
	this->mOrderedCategories.resize(categoryCount);
	CategoryMeta* prevCategory = nullptr, *category;
	auto voxelIdIter = voxelIds.begin();
	for (uIndex i = 0; i < categoryCount; ++i)
	{
		category = &mOrderedCategories[i];
		*category = { *voxelIdIter, buffer, 0, 0, i32(i), prevCategory, nullptr };
		voxelIdIter++;
		if (prevCategory != nullptr)
		{
			prevCategory->nextCategory = category;
		}
		this->mCategoryById.insert(std::make_pair(categories[i], category));
		prevCategory = category;
	}
	this->mEmpty = { std::nullopt, buffer, 0, 0, i32(categoryCount), prevCategory, &this->mUnallocated };
	prevCategory->nextCategory = &this->mEmpty;
	this->mUnallocated = { std::nullopt, buffer, 0, totalValueCount, -1, &this->mEmpty, nullptr };
	this->mCoordinateToIndex.reserve(totalValueCount);
	this->mIndexToCoordinate.resize(totalValueCount);
}

void VoxelInstanceCategoryList::clear()
{
	this->mCoordinateToIndex.clear();
	this->mIndexToCoordinate.clear();
	this->mCategoryById.clear();
	this->mOrderedCategories.clear();
	this->mEmpty = {};
	this->mUnallocated = {};
}

uSize VoxelInstanceCategoryList::unallocatedCount() const
{
	return this->mUnallocated.count;
}

uIndex VoxelInstanceCategoryList::allocateToEmpty()
{
	// ASSERT: The empty voxels list is immediately before the unallocated voxels list
	assert(this->mEmpty.index + this->mEmpty.count == this->mUnallocated.index);
	// Push the value allocation into the set of "empty" voxels
	this->mEmpty.expandRight();
	return this->mEmpty.lastIndex();
}

void VoxelInstanceCategoryList::deallocateFromEmpty(uSize amount)
{
	while (amount-- > 0)
	{
		this->mUnallocated.expandLeft();
	}
}

CategoryMeta& VoxelInstanceCategoryList::getCategoryForValueIndex(uIndex idx)
{
	OPTICK_EVENT();
	if (this->mEmpty.containsIndex(idx))
	{
		return this->mEmpty;
	}
	else if (this->mUnallocated.containsIndex(idx))
	{
		return this->mUnallocated;
	}
	else
	{
		// TODO: Can be optimized by binary searching the ordered list by the idx and count of each category
		for (auto& category : this->mOrderedCategories)
		{
			if (category.containsIndex(idx))
			{
				return category;
			}
		}
	}
	assert(false);
	return this->mUnallocated;
}

CategoryMeta& VoxelInstanceCategoryList::getCategoryForId(std::optional<game::BlockId> id)
{
	return id ? *this->mCategoryById.at(*id) : this->mEmpty;
}

CategoryMeta& VoxelInstanceCategoryList::getCategory(uIndex categoryIndex)
{
	return this->mOrderedCategories[categoryIndex];
}

void VoxelInstanceCategoryList::setCoordinateIndex(world::Coordinate const& coordinate, uIndex idx)
{
	OPTICK_EVENT();
	auto lastThatIsGreater = std::upper_bound(this->mCoordinateToIndex.begin(), this->mCoordinateToIndex.end(), coordinate, ValueCoordinateComparator{});
	this->mCoordinateToIndex.insert(lastThatIsGreater, std::pair(coordinate, idx));
	this->mIndexToCoordinate[idx] = coordinate;
}

void VoxelInstanceCategoryList::moveIndexedCoordinate(uIndex const& src, uIndex const& dst)
{
	OPTICK_EVENT();
	auto coordinateOpt = this->mIndexToCoordinate[src];
	assert(coordinateOpt);
	auto iterOpt = this->searchForCoordinateIterator(*coordinateOpt);
	assert(iterOpt);
	(*iterOpt)->second = dst;
	this->mIndexToCoordinate[dst] = coordinateOpt;
	this->mIndexToCoordinate[src] = std::nullopt;
}

void VoxelInstanceCategoryList::removeCoordinateIndex(world::Coordinate const& coordinate)
{
	OPTICK_EVENT();
	if (auto iterOpt = this->searchForCoordinateIterator(coordinate))
	{
		this->mIndexToCoordinate[(*iterOpt)->second] = std::nullopt;
		this->mCoordinateToIndex.erase(*iterOpt);
	}
}

std::optional<VoxelInstanceCategoryList::CoordinateToIndexList::const_iterator> VoxelInstanceCategoryList::searchForCoordinateIterator(
	world::Coordinate const& coordinate
) const
{
	OPTICK_EVENT();
	
	auto rangeBegin = this->mCoordinateToIndex.begin();
	auto rangeEnd = this->mCoordinateToIndex.end();
	auto dst = std::distance(rangeBegin, rangeEnd);
	assert(dst == this->mCoordinateToIndex.size());
	if (dst == 0) return std::nullopt;

	auto range = std::equal_range(rangeBegin, rangeEnd, coordinate, ValueCoordinateComparator{});
	if (std::distance(range.first, range.second) > 0 && range.first != rangeEnd)
	{
		return range.first;
	}
	else
	{
		return std::nullopt;
	}
}

std::optional<VoxelInstanceCategoryList::CoordinateToIndexList::iterator> VoxelInstanceCategoryList::searchForCoordinateIterator(
	world::Coordinate const& coordinate
)
{
	OPTICK_EVENT();

	auto rangeBegin = this->mCoordinateToIndex.begin();
	auto rangeEnd = this->mCoordinateToIndex.end();
	auto dst = std::distance(rangeBegin, rangeEnd);
	assert(dst == this->mCoordinateToIndex.size());
	if (dst == 0) return std::nullopt;

	auto range = std::equal_range(rangeBegin, rangeEnd, coordinate, ValueCoordinateComparator{});
	if (std::distance(range.first, range.second) > 0 && range.first != rangeEnd)
	{
		return range.first;
	}
	else
	{
		return std::nullopt;
	}
}

std::optional<uIndex> VoxelInstanceCategoryList::searchForCoordinateIndex(world::Coordinate const& coordinate) const
{
	OPTICK_EVENT();
	if (auto iterOpt = this->searchForCoordinateIterator(coordinate))
	{
		return (*iterOpt)->second;
	}
	return std::nullopt;
}

graphics::AttributeBinding BlockInstanceBuffer::getBinding(ui8& slot)
{
	auto modelMatrix = (ui32)offsetof(ValueData, model);
	auto vec4size = (ui32)sizeof(math::Vector4);
	// Data per object instance - this is only for objects which dont more, rotate, or scale
	return graphics::AttributeBinding(graphics::AttributeBinding::Rate::eInstance)
		.setStructType<ValueData>()
		.addAttribute({ /*slot*/ slot++, /*vec3*/ (ui32)vk::Format::eR32G32B32Sfloat, offsetof(ValueData, posOfChunk) })
		// mat4 using 4 slots
		.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, modelMatrix + (0 * vec4size) })
		.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, modelMatrix + (1 * vec4size) })
		.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, modelMatrix + (2 * vec4size) })
		.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, modelMatrix + (3 * vec4size) })
		.addAttribute({ /*slot*/ slot++, /*vec4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(ValueData, flags) })
		;
}

BlockInstanceBuffer::BlockInstanceBuffer(uSize totalValueCount, std::unordered_set<game::BlockId> const& voxelIds)
	: mTotalInstanceCount(totalValueCount)
	, mMutableCategoryList(totalValueCount, &mInstanceBuffer, voxelIds)
{
	OPTICK_EVENT();
	OPTICK_TAG("InstanceCount", totalValueCount);

	this->mInstanceData = (ValueData*)malloc(this->size());

	this->mStagingBuffer.setUsage(vk::BufferUsageFlagBits::eTransferSrc, graphics::MemoryUsage::eCPUSource);
	this->mStagingBuffer.setSize(BlockInstanceBuffer::stagingBufferSize());

	this->mInstanceBuffer.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, graphics::MemoryUsage::eGPUOnly);
	this->mInstanceBuffer.setSize(this->size());

	for (auto const& id : voxelIds)
	{
		this->mCommittedCategories.insert(std::make_pair(id, CategoryMeta{}));
	}
}

BlockInstanceBuffer::~BlockInstanceBuffer()
{
	this->mMutableCategoryList.clear();
	this->mCommittedCategories.clear();

	this->mInstanceBuffer.destroy();
	this->mStagingBuffer.destroy();

	free(this->mInstanceData);
	this->mInstanceData = nullptr;

	this->mTotalInstanceCount = 0;
}

uSize BlockInstanceBuffer::size() const
{
	return this->mTotalInstanceCount * sizeof(ValueData);
}

void BlockInstanceBuffer::lock()
{
	this->mMutex.lock();
}

void BlockInstanceBuffer::unlock()
{
	this->mMutex.unlock();
}

void BlockInstanceBuffer::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	this->mInstanceBuffer.setDevice(device);
	this->mStagingBuffer.setDevice(device);
}

void BlockInstanceBuffer::createBuffer()
{
	OPTICK_EVENT();
	this->mInstanceBuffer.create();
	this->mStagingBuffer.create();
}

void BlockInstanceBuffer::allocateCoordinates(std::vector<world::Coordinate> const& coordinates)
{
	OPTICK_EVENT();

	// TODO: Assert that the coordinate does not already exist in the list

	// ASSERT: There is enough room to allocate the coordinates
	assert(coordinates.size() <= this->mMutableCategoryList.unallocatedCount());

	for (auto const& coordinate : coordinates)
	{
		// Push the value allocation into the set of "empty" voxels
		auto valueIndex = this->mMutableCategoryList.allocateToEmpty();

		// Put the coordinate in the searchable sorted list of allocated voxels
		this->mMutableCategoryList.setCoordinateIndex(coordinate, valueIndex);

		auto* instance = this->getInstanceAt(valueIndex);
		instance->posOfChunk = math::Vector3Padded(coordinate.chunk().toFloat());
		instance->model = math::translate(coordinate.local().toFloat());
	}
}

void BlockInstanceBuffer::unallocateCoordinates(std::vector<world::Coordinate> const& coordinates)
{
	OPTICK_EVENT();

	// Move all coordinates to the empty set (thereby putting them all at the end of the empty category)
	for (auto const& coordinate : coordinates)
	{
		// Assumes that this results in the data for coordinate being in the LAST index for the empty category
		this->changeVoxelId(coordinate, std::nullopt);
	}
	for (auto const& coordinate : coordinates)
	{
		this->mMutableCategoryList.removeCoordinateIndex(coordinate);
	}
	this->mMutableCategoryList.deallocateFromEmpty(coordinates.size());
}

BlockInstanceBuffer::ValueData* BlockInstanceBuffer::getInstanceAt(uIndex idx)
{
	assert(idx < this->mTotalInstanceCount);
	return this->mInstanceData + idx;
}

void BlockInstanceBuffer::changeVoxelId(world::Coordinate const& coordinate, std::optional<game::BlockId> const desiredVoxelId)
{
	OPTICK_EVENT();
	
	auto const instanceIndexOpt = this->mMutableCategoryList.searchForCoordinateIndex(coordinate);
	assert(instanceIndexOpt);
	auto instanceIndex = *instanceIndexOpt;

	// copy out the instance we want to move so its safe to write to `prevInstanceIndex`
	ValueData instance = *this->getInstanceAt(instanceIndex);
	auto& srcCategory = this->mMutableCategoryList.getCategoryForValueIndex(instanceIndex);
	auto& destCategory = this->mMutableCategoryList.getCategoryForId(desiredVoxelId);
	if (srcCategory.categoryIndex == destCategory.categoryIndex) return;

	this->mMutableCategoryList.removeCoordinateIndex(coordinate);

	if (destCategory.categoryIndex < srcCategory.categoryIndex) // dest <- src (later in mem to earlier)
	{
		/* Perform the following order of operations, where `instanceIndex` is `T` (whose data is in `instance`)
			| destCategory | category1 | category2 | srcCategory |
			| A  B  C  D   | G H I J K | L M N O P | Q R S T U V |
			                                               ^
		*/

		/* Move the instance at the start of the category to the index of the moving-value
			| destCategory | category1 | category2 | srcCategory |
			| A  B  C  D   | G H I J K | L M N O P | Q R S Q U V |
			                                         ^ --> ^
		*/
		if (instanceIndex != srcCategory.firstIndex())
		{
			this->copyInstanceData(srcCategory.firstIndex(), instanceIndex);
			this->mMutableCategoryList.moveIndexedCoordinate(srcCategory.firstIndex(), instanceIndex);
		}

		auto* rightCate = &srcCategory;
		auto* leftCate = rightCate->prevCategory;
		while (leftCate != &destCategory)
		{
			/* LoopInit
				Phase 1:
					rightCate = srcCategory
					leftCate = category2
				Phase 2:
					rightCate = category2
					leftCate = category1
			*/

			if (leftCate->count > 0)
			{
				/* Move the data in the first index of the prev category into the first index of the next category
					Phase 1:
					| destCategory | category1 | category2 | srcCategory |
					| A  B  C  D   | G H I J K | L M N O P | L R S Q U V |
																			 ^ --------> ^
					Phase 2:
					| destCategory | category1 | category2   | srcCategory |
					| A  B  C  D   | G H I J K | G M N O P L |  R S Q U V  |
													 ^ --------> ^
				*/
				this->copyInstanceData(leftCate->firstIndex(), rightCate->firstIndex());
				this->mMutableCategoryList.moveIndexedCoordinate(leftCate->firstIndex(), rightCate->firstIndex());
			
				/* Shift category bounds to keep the moved data in the correct category
					Phase 1:
					| destCategory | category1 | category2   | srcCategory |
					| A  B  C  D   | G H I J K | L M N O P L |  R S Q U V  |
					Phase 2:
					| destCategory | category1   | category2 | srcCategory |
					| A  B  C  D   | G H I J K G | M N O P L |  R S Q U V  |
				*/
				leftCate->expandRight();
			}

			/*
				Phase 1:
					rightCate = category2
					leftCate = category1
				Phase 2:
					rightCate = category1
					leftCate = destCategory
			*/
			rightCate = leftCate;
			leftCate = leftCate->prevCategory;
		}

		/* Finally, shift the barrier of dest category into rightCate...
			| destCategory | category1 | category2 | srcCategory |
			|  A B C D   G | H I J K G | M N O P L |  R S Q U V  |
		*/
		leftCate->expandRight();

		/* so the last index of destCategory can be filled with `T`/`instance`.
			| destCategory | category1 | category2 | srcCategory |
			|  A B C D   T | H I J K G | M N O P L |  R S Q U V  |
		*/
		this->setInstanceData(leftCate->lastIndex(), &instance);
		this->mMutableCategoryList.setCoordinateIndex(coordinate, leftCate->lastIndex());
	}
	else // dest -> src (earlier in mem to later)
	{
		/* Perform the following order of operations, where `instanceIndex` is `B` (whose data is in `instance`)
			| srcCategory | category1 | category2 | destCategory |
			| A B C D E F | G H I J K | L M N O P | Q R S T U V  |
			    ^
		*/

		/* Move the instance at the end of the category to the index of the moving-value
			| srcCategory | category1 | category2 | destCategory |
			| A F C D E F | G H I J K | L M N O P | Q R S T U V  |
					^ <---- ^
		*/
		if (instanceIndex != srcCategory.lastIndex())
		{
			this->copyInstanceData(srcCategory.lastIndex(), instanceIndex);
			this->mMutableCategoryList.moveIndexedCoordinate(srcCategory.lastIndex(), instanceIndex);
		}

		auto* leftCate = &srcCategory;
		auto* rightCate = leftCate->nextCategory;
		while (rightCate != &destCategory)
		{
			/* LoopInit
				Phase 1:
					leftCate = srcCategory
					rightCate = category1
				Phase 2:
					leftCate = category1
					rightCate = category2
			*/

			if (rightCate->count > 0)
			{
				/* Move the data in the first index of the prev category into the first index of the next category
					Phase 1:
					| srcCategory | category1 | category2 | destCategory |
					| A F C D E K | G H I J K | L M N O P | Q R S T U V  |
					            ^ <-------- ^
					Phase 2:
					| srcCategory | category1   | category2 | destCategory |
					| A F C D E   | K G H I J P | L M N O P | Q R S T U V  |
													          ^ <-------- ^
				*/
				this->copyInstanceData(rightCate->lastIndex(), leftCate->lastIndex());
				this->mMutableCategoryList.moveIndexedCoordinate(rightCate->lastIndex(), leftCate->lastIndex());

				/* Shift category bounds to keep the moved data in the correct category
					Phase 1:
					| srcCategory | category1   | category2 | destCategory |
					| A F C D E   | K G H I J K | L M N O P | Q R S T U V  |
					Phase 2:
					| srcCategory | category1 | category2   | destCategory |
					| A F C D E   | K G H I J | P L M N O P | Q R S T U V  |
				*/
				rightCate->expandLeft();
			}

			/*
				Phase 1:
					leftCate = category1
					rightCate = category2
				Phase 2:
					leftCate = category2
					rightCate = destCategory
			*/
			leftCate = rightCate;
			rightCate = rightCate->nextCategory;
		}

		/* Finally, shift the barrier of dest category into rightCate...
			| srcCategory | category1 | category2 | destCategory  |
			| A F C D E   | K G H I J | P L M N O | P Q R S T U V |
		*/
		rightCate->expandLeft();

		/* copy the last index of destCategory to the first
			| srcCategory | category1 | category2 | destCategory  |
			| A F C D E   | K G H I J | P L M N O | V Q R S T U V |
					                                    ^ <-------- ^
		*/
		this->copyInstanceData(rightCate->lastIndex(), rightCate->firstIndex());
		this->mMutableCategoryList.moveIndexedCoordinate(rightCate->lastIndex(), rightCate->firstIndex());

		/* so the last index of destCategory can be filled with `B`/`instance`.
			| srcCategory | category1 | category2 | destCategory  |
			| A F C D E   | K G H I J | P L M N O | V Q R S T U B |
																							            ^
		*/
		this->setInstanceData(rightCate->lastIndex(), &instance);
		this->mMutableCategoryList.setCoordinateIndex(coordinate, rightCate->lastIndex());
	}
	
	instance.posOfChunk.x() = 0;
}

/**
 * Copies data in `mInstanceData` from the index `src` to `dest`,
 * marking the `dest` index in `mChangedBufferIndices` in the process
 */
void BlockInstanceBuffer::copyInstanceData(uIndex const& src, uIndex const& dest)
{
	ValueData* srcData = this->getInstanceAt(src);
	this->setInstanceData(dest, srcData);
}

void BlockInstanceBuffer::setInstanceData(uIndex const& idx, ValueData const *const data)
{
	auto* ptr = this->getInstanceAt(idx);
	*ptr = *data;
	this->mChangedBufferIndices.insert(idx);
}

bool BlockInstanceBuffer::hasChanges() const
{
	return this->mChangedBufferIndices.size() > 0;
}

void BlockInstanceBuffer::commitToBuffer(graphics::CommandPool* transientPool)
{
	OPTICK_EVENT();
	static const uSize valueSize = sizeof(ValueData);

	auto regionsToCopy = std::vector<graphics::BufferRegionCopy>();
	uSize indicesPrepared = 0;
	std::optional<uIndex> prevIdx = std::nullopt;
	graphics::BufferRegionCopy regionBeingPrepared = { 0, 0, 0 };
	uSize totalStagingBufferSizeWritten = 0;
	while (
		this->hasChanges() && (totalStagingBufferSizeWritten + valueSize) <= BlockInstanceBuffer::stagingBufferSize()
	)
	{
		// Because `mChangedBufferIndices` is a sorted-set,
		// there are no duplicates and the next entry
		// will always have a larger/later index than the previous
		auto nextEntry = this->mChangedBufferIndices.begin();
		assert(nextEntry != this->mChangedBufferIndices.end());
		auto nextInstanceIdx = *nextEntry;
		ValueData* nextInstance = this->getInstanceAt(nextInstanceIdx);

		// If the previous entry and next entry are contiguous (next to each other in the buffer)
		// the the current region can be expanded instead of adding a new region
		if (prevIdx.has_value() && nextInstanceIdx == *prevIdx + 1)
		{
			// If the current idx is contiguous, we can just append the size.
			// The instance will be written below next to the previous entry
			// regardless of if they are actually contiguous.
			regionBeingPrepared.size += valueSize;
		}
		// The entries are not contiguous, so we must append the previous region and start a new one
		else
		{
			if (regionBeingPrepared.size > 0)
			{
				regionsToCopy.push_back(regionBeingPrepared);
			}
			regionBeingPrepared = {
				/*srcOffset*/ totalStagingBufferSizeWritten,
				/*dstOffset*/ nextInstanceIdx * valueSize,
				valueSize
			};
		}

		// Actually write the instance to the staging buffer
		this->mStagingBuffer.write(
			totalStagingBufferSizeWritten,
			nextInstance, valueSize, false
		);
		totalStagingBufferSizeWritten += valueSize;
		
		prevIdx = nextInstanceIdx;
		indicesPrepared++;

		this->mChangedBufferIndices.erase(nextEntry);
	}
	if (regionBeingPrepared.size > 0)
	{
		regionsToCopy.push_back(regionBeingPrepared);
	}

	// Write the regions to the GPU memory of the Instance buffer
	auto pStaging = &mStagingBuffer;
	auto pGPUBuffer = &mInstanceBuffer;
	/*
		TODO: submitOneOff contains a waitIdle at the end of the call. This causes all commands to be synchronous.
		For higher throughput, waitIdle should not be called, and the command buffers should rely entirely
		on the pipeline barriers. That way the next frame submit can just wait (via fences/semaphores)
		for this to finish on GPU side instead of waiting in this function.
	*/
	transientPool->submitOneOff([&pStaging, &pGPUBuffer, &regionsToCopy](graphics::Command* cmd) {
		cmd->copyBuffer(pStaging, pGPUBuffer, regionsToCopy);
	});

	// If there are additional changes (assuming `submitOneOff` halts this function until the change is final)
	// then we should continue committing until there are no other changes.
	if (this->hasChanges())
	{
		this->commitToBuffer(transientPool);
		return;
	}

	// Now that there are no longer any changes, we can expose the new category sizes
	for (auto& entry : this->mCommittedCategories)
	{
		entry.second = this->mMutableCategoryList.getCategoryForId(entry.first);
	}
}

CategoryMeta const& BlockInstanceBuffer::getDataForVoxelId(game::BlockId const& id) const
{
	return this->mCommittedCategories.at(id);
}

void BlockInstanceBuffer::ValueData::setFaceVisible(ui8 axis, ui8 direction, bool visible)
{
	ui32* faceVisibility = reinterpret_cast<ui32*>(&this->flags[0]);
	auto mask = ModelVoxel::makeFaceBitMask(axis, direction);
	if (visible)
	{
		(*faceVisibility) |= mask;
	}
	else
	{
		(*faceVisibility) &= ~mask;
	}
}

void BlockInstanceBuffer::onLoadingChunk(math::Vector3Int const& coordinate)
{
	auto coordinates = std::vector<world::Coordinate>();
	FOR_CHUNK_SIZE(i32, y) FOR_CHUNK_SIZE(i32, z) FOR_CHUNK_SIZE(i32, x)
	{
		coordinates.push_back(world::Coordinate(coordinate, { x, y, z }));
	}
	this->lock();
	this->allocateCoordinates(coordinates);
	this->unlock();
}

void BlockInstanceBuffer::onUnloadingChunk(math::Vector3Int const& coordinate)
{
	auto coordinates = std::vector<world::Coordinate>();
	FOR_CHUNK_SIZE(i32, y) FOR_CHUNK_SIZE(i32, z) FOR_CHUNK_SIZE(i32, x)
	{
		coordinates.push_back(world::Coordinate(coordinate, { x, y, z }));
	}
	this->lock();
	this->unallocateCoordinates(coordinates);
	this->unlock();
}

void BlockInstanceBuffer::onVoxelsChanged(TChangedVoxelsList const& changes)
{
	OPTICK_EVENT()
		this->lock();
	for (auto const& change : changes)
	{
		this->changeVoxelId(change.first, change.second);
	}
	for (auto const& change : changes)
	{
		this->setNeighborFaceVisibility(change.first, !change.second.has_value());
	}
	this->unlock();
}

void BlockInstanceBuffer::setNeighborFaceVisibility(world::Coordinate const& coordinate, bool const& bIsEmpty)
{
	OPTICK_EVENT()
	
	auto const idxInstance = this->mMutableCategoryList.searchForCoordinateIndex(coordinate);
	assert(idxInstance);
	this->mChangedBufferIndices.insert(*idxInstance);
	auto* changedInstance = this->getInstanceAt(*idxInstance);

	for (ui8 axis = 0; axis < 3; ++axis)
	{
		for (ui8 direction = 0; direction < 2; ++direction)
		{
			auto offset = math::Vector3Int(0);
			offset[axis] = direction == 0 ? -1 : 1;
			auto neighborCoord = coordinate + offset;

			auto const neighborInstanceIdx = this->mMutableCategoryList.searchForCoordinateIndex(neighborCoord);
			std::optional<game::BlockId> neighborId = std::nullopt;
			if (neighborInstanceIdx.has_value()) // neighbor may be in a chunk that is not loaded
			{
				auto& neighborCategory = this->mMutableCategoryList.getCategoryForValueIndex(*neighborInstanceIdx);
				neighborId = neighborCategory.id;

				if (neighborId)
				{
					this->mChangedBufferIndices.insert(*neighborInstanceIdx);
					auto* neighborInstance = this->getInstanceAt(*neighborInstanceIdx);
					// the face being set is in the opposite direction
					neighborInstance->setFaceVisible(axis, 1 - direction, bIsEmpty);
				}
			}

			changedInstance->setFaceVisible(axis, direction, bIsEmpty || !neighborId.has_value());
		}
	}
}
