#include "ecs/component/ComponentManager.hpp"

#include "ecs/Core.hpp"
#include "logging/Logger.hpp"

using namespace ecs;


struct TestType
{

	static constexpr uSize maxPoolSize() { return 5; }

};

template <uSize Capacity>
class Pool
{
	ui32 pool[Capacity];
};


ComponentManager::ComponentManager(Core *pCore)
	: mpCore(pCore)
	, mpPoolMemory(nullptr)
	, mRegisteredTypeCount(0)
{
	auto testPool = Pool<TestType::maxPoolSize()>();
}

ComponentManager::~ComponentManager()
{
	if (this->mpPoolMemory != nullptr)
	{
		free(this->mpPoolMemory);
		this->mpPoolMemory = nullptr;
	}
}

void ComponentManager::registerType(ComponentTypeId const& id, TypeMetadata const& metadata)
{
	this->mMetadataByType[id] = metadata;
	this->mRegisteredTypeCount++;
	this->mpCore->log(
		LOG_INFO, "Registered ECS component type %s with id %i and size %i",
		metadata.name.c_str(), id, metadata.size
	);
}

void ComponentManager::allocatePools()
{
	uSize sumSizeOfAllPools = 0;
	uSize sumSizeOfAllIdLists = 0;
	for (uSize i = 0; i < this->mRegisteredTypeCount; ++i)
	{
		sumSizeOfAllPools += this->mMetadataByType[i].objectPoolSize;
		sumSizeOfAllIdLists += this->mMetadataByType[i].availableIdArraySize;
	}

	this->mpCore->log(LOG_INFO, "Allocating %i bytes for ecs component pools", sumSizeOfAllPools);
	this->mpCore->log(LOG_INFO, "Allocating %i bytes for ecs component id-tracking", sumSizeOfAllIdLists);

	// this memory chunk should now have enough room for:
	// 1) memory manager header
	// 2) some number of object pools equivalent to the number of types registered
	this->mpPoolMemory = malloc(sumSizeOfAllPools);
	this->mpAvailableIdMemory = malloc(sumSizeOfAllIdLists);

	// NOTE: Assumes each pool can be initialized by clearing its memory to 0
	memset(this->mpPoolMemory, 0, sumSizeOfAllPools);
	memset(this->mpAvailableIdMemory, 0, sumSizeOfAllIdLists);

	uSize nextPoolPtrLoc = (uSize)this->mpPoolMemory;
	uSize nextIdListPtrLoc = (uSize)this->mpAvailableIdMemory;
	for (ComponentTypeId typeId = 0; typeId < this->mRegisteredTypeCount; ++typeId)
	{
		this->mPoolByType[typeId] = (void*)nextPoolPtrLoc;
		nextPoolPtrLoc += this->mMetadataByType[typeId].objectPoolSize;
		this->mAvailableIdsByType[typeId] = (void*)nextIdListPtrLoc;
		nextIdListPtrLoc += this->mMetadataByType[typeId].availableIdArraySize;
	}
}
