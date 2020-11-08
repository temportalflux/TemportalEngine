#include "world/BlockInstanceMap.hpp"

#include "graphics/Command.hpp"
#include "graphics/Memory.hpp"

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
	return this->firstIndex() <= idx && idx <= this->lastIndex();
}

void CategoryMeta::expandLeft()
{
	if (this->prevCategory != nullptr) this->prevCategory->shrinkLeft();
	this->count++;
	decrementStart();
}

void CategoryMeta::shrinkLeft()
{
	if (this->count > 0)
	{
		this->count--;
		if (this->nextCategory != nullptr)
			this->nextCategory->updateIndex();
	}
	else if (this->prevCategory != nullptr)
	{
		this->prevCategory->shrinkLeft();
		updateIndex();
	}
}

void CategoryMeta::expandRight()
{
	this->count++;
	if (this->nextCategory != nullptr)
	{
		this->nextCategory->updateIndex();
		this->nextCategory->shrinkRight();
	}
}

void CategoryMeta::shrinkRight()
{
	if (this->count > 0)
	{
		this->count--;
		this->incrementStart();
	}
	else if (this->nextCategory != nullptr)
	{
		this->nextCategory->shrinkRight();
	}
}

void CategoryMeta::decrementStart()
{
	this->index--;
	if (this->nextCategory != nullptr)
		this->nextCategory->updateIndex();
}

void CategoryMeta::incrementStart()
{
	this->index++;
	if (this->nextCategory != nullptr)
		this->nextCategory->updateIndex();
}

void CategoryMeta::updateIndex()
{
	if (this->count == 0)
	{
		this->index = this->prevCategory->lastIndex() + 1;
		if (this->nextCategory != nullptr)
			this->nextCategory->updateIndex();
	}
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
	for (uIndex i = 0; i < categoryCount; ++i)
	{
		category = &mOrderedCategories[i];
		*category = { buffer, 0, 0, i32(i), prevCategory, nullptr };
		if (prevCategory != nullptr)
		{
			prevCategory->nextCategory = category;
		}
		this->mCategoryById.insert(std::make_pair(categories[i], category));
		prevCategory = category;
	}
	this->mEmpty = { buffer, 0, 0, i32(categoryCount), prevCategory, &this->mUnallocated };
	prevCategory->nextCategory = &this->mEmpty;
	this->mUnallocated = { buffer, 0, totalValueCount, -1, &this->mEmpty, nullptr };
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

uIndex VoxelInstanceCategoryList::allocate()
{
	// ASSERT: The empty voxels list is immediately before the unallocated voxels list
	assert(this->mEmpty.index + this->mEmpty.count == this->mUnallocated.index);
	// Push the value allocation into the set of "empty" voxels
	this->mEmpty.expandRight();
	return this->mEmpty.lastIndex();
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
	auto rangeEnd = this->mCoordinateToIndex.end();
	auto range = std::equal_range(this->mCoordinateToIndex.begin(), rangeEnd, coordinate, ValueCoordinateComparator{});
	if (range.first != rangeEnd && range.second != rangeEnd)
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
		.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, modelMatrix + (3 * vec4size) });
}

BlockInstanceBuffer::BlockInstanceBuffer(uSize totalValueCount, std::unordered_set<game::BlockId> const& voxelIds)
	: mTotalInstanceCount(totalValueCount)
	, mMutableCategoryList(totalValueCount, &mInstanceBuffer, voxelIds)
{
	OPTICK_EVENT();
	OPTICK_TAG("InstanceCount", totalValueCount);

	this->mInstanceData = (ValueData*)malloc(this->size());

	this->mpMemory = std::make_shared<graphics::Memory>();
	this->mpMemory->setFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);

	this->mInstanceBuffer.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);
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
	this->mpMemory.reset();

	free(this->mInstanceData);
	this->mInstanceData = nullptr;

	this->mTotalInstanceCount = 0;
}

uSize BlockInstanceBuffer::size() const
{
	return this->mTotalInstanceCount * sizeof(ValueData);
}

void BlockInstanceBuffer::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	this->mpMemory->setDevice(device);
	this->mInstanceBuffer.setDevice(device);
}

void BlockInstanceBuffer::createBuffer()
{
	OPTICK_EVENT();
	this->mInstanceBuffer.create();
	this->mInstanceBuffer.configureSlot(this->mpMemory);

	this->mpMemory->create();

	this->mInstanceBuffer.bindMemory();
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
		auto valueIndex = this->mMutableCategoryList.allocate();

		// Put the coordinate in the searchable sorted list of allocated voxels
		this->mMutableCategoryList.setCoordinateIndex(coordinate, valueIndex);

		auto* instance = this->getInstanceAt(valueIndex);
		instance->posOfChunk = math::Vector3Padded(coordinate.chunk().toFloat());
		instance->model = math::createModelMatrix(coordinate.local().toFloat());
	}
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

	if (destCategory.categoryIndex < srcCategory.categoryIndex)
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
		this->copyInstanceData(srcCategory.firstIndex(), instanceIndex);

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
	}
	else // dest -> src (prev in mem to later)
	{
		// TODO
	}

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

void BlockInstanceBuffer::commitToBuffer()
{
	OPTICK_EVENT();

}

CategoryMeta const& BlockInstanceBuffer::getDataForVoxelId(game::BlockId const& id) const
{
	return this->mCommittedCategories.at(id);
}
