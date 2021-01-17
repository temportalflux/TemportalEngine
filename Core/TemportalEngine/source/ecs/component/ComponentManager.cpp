#include "ecs/component/ComponentManager.hpp"

#include "ecs/Core.hpp"
#include "network/packet/NetworkPacketECSReplicate.hpp"
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

IEVCSObject* Manager::createObject(TypeId const& typeId)
{
	return this->create(typeId);
}

Component* Manager::create(ComponentTypeId const& typeId)
{
	uIndex objectId;
	auto ptr = reinterpret_cast<Component*>(
		this->mPoolByType[typeId].create(objectId)
	);
	ptr->id = objectId;
	this->mMetadataByType[typeId].construct(ptr);
	this->mAllocatedByType[typeId].insert(std::make_pair(objectId, ptr));

	if (ecs::Core::Get()->shouldReplicate())
	{
		ptr->netId = this->nextNetworkId();
		ecs::Core::Get()->replicateCreate()
			->setObjectEcsType(ecs::EType::eComponent)
			.setObjectTypeId(typeId)
			.setObjectNetId(ptr->netId)
			;
	}

	return ptr;
}

Component* Manager::get(ComponentTypeId const& typeId, Identifier const& id)
{
	auto iter = this->mAllocatedByType[typeId].find(id);
	assert(iter != this->mAllocatedByType[typeId].end());
	return iter->second;
}

void Manager::destroy(ComponentTypeId const& typeId, Identifier const& id)
{
	auto iter = this->mAllocatedByType[typeId].find(id);
	assert(iter != this->mAllocatedByType[typeId].end());

	if (ecs::Core::Get()->shouldReplicate())
	{
		ecs::Core::Get()->replicateDestroy()
			->setObjectEcsType(ecs::EType::eComponent)
			.setObjectTypeId(typeId)
			.setObjectNetId(iter->second->id)
			;
	}

	this->mAllocatedByType[typeId].erase(iter);
	this->mPoolByType[typeId].destroy(id);
}

Component* Manager::getNetworked(ComponentTypeId const& typeId, Identifier const& netId)
{
	return this->get(typeId, this->getNetworkedObjectId(netId));
}
