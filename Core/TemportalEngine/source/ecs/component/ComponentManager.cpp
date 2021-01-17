#include "ecs/component/ComponentManager.hpp"

#include "ecs/Core.hpp"
#include "logging/Logger.hpp"

using namespace ecs;
using namespace ecs::component;

Manager::Manager(Core *pCore)
	: mpCore(pCore)
	, mpPoolMemory(nullptr)
	, mRegisteredTypeCount(0)
{
}

Manager::~Manager()
{
	if (this->mpPoolMemory != nullptr)
	{
		free(this->mpPoolMemory);
		this->mpPoolMemory = nullptr;
	}
}

void Manager::registerType(ComponentTypeId const& id, TypeMetadata const& metadata)
{
	this->mMetadataByType[id] = metadata;
	this->mRegisteredTypeCount++;
	this->mpCore->log(
		LOG_INFO, "Registered ECS component type %s with id %i and size %i",
		metadata.name.c_str(), id, metadata.size
	);
}

void Manager::allocatePools()
{
	uSize sumSizeOfAllPools = 0;
	for (uSize i = 0; i < this->mRegisteredTypeCount; ++i)
	{
		auto const& metadata = this->mMetadataByType[i];
		sumSizeOfAllPools += (metadata.size * metadata.objectCount);
	}

	this->mpCore->log(LOG_INFO, "Allocating %i bytes for ecs component pools", sumSizeOfAllPools);

	// this memory chunk should now have enough room for:
	// 1) memory manager header
	// 2) some number of object pools equivalent to the number of types registered
	this->mpPoolMemory = malloc(sumSizeOfAllPools);
	// NOTE: Assumes each pool can be initialized by clearing its memory to 0
	memset(this->mpPoolMemory, 0, sumSizeOfAllPools);

	uSize nextPoolPtrLoc = (uSize)this->mpPoolMemory;
	for (ComponentTypeId typeId = 0; typeId < this->mRegisteredTypeCount; ++typeId)
	{
		auto const& metadata = this->mMetadataByType[typeId];
		this->mPoolByType[typeId]
			.init(metadata.size, metadata.objectCount)
			.assignMemory((void*)nextPoolPtrLoc);
		nextPoolPtrLoc += this->mPoolByType[typeId].memSize();
	}
}

std::shared_ptr<Component> Manager::create(ComponentTypeId const& typeId)
{
	uIndex objectId;
	auto ptr = std::shared_ptr<Component>(
		reinterpret_cast<Component*>(
			this->mPoolByType[typeId].create(sizeof(Component), objectId)
		),
		std::bind(&Manager::destroy, this, typeId, std::placeholders::_1)
	);
	ptr->id = objectId;
	this->mAllocatedByType[typeId].insert(std::make_pair(objectId, ptr));
	return ptr;
}

std::shared_ptr<Component> Manager::get(ComponentTypeId const& typeId, Identifier const& id)
{
	auto weakIter = this->mAllocatedByType[typeId].find(id);
	assert(weakIter != this->mAllocatedByType[typeId].end());
	assert(!weakIter->second.expired());
	return weakIter->second.lock();
}

std::shared_ptr<Component> Manager::getNetworked(ComponentTypeId const& typeId, Identifier const& netId)
{
	return this->get(typeId, this->getNetworkedObjectId(netId));
}

void Manager::destroy(ComponentTypeId const& typeId, Component *pCreated)
{
	auto iter = this->mAllocatedByType[typeId].find(pCreated->id);
	assert(iter != this->mAllocatedByType[typeId].end());
	this->mAllocatedByType[typeId].erase(iter);
	this->mPoolByType[typeId].destroy(pCreated->id);
}
