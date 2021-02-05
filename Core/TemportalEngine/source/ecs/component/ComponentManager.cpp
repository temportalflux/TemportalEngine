#include "ecs/component/ComponentManager.hpp"

#include "ecs/Core.hpp"
#include "network/packet/NetworkPacketECSReplicate.hpp"
#include "logging/Logger.hpp"

using namespace evcs;
using namespace evcs::component;

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

std::string Manager::typeName(TypeId const& typeId) const
{
	return this->mMetadataByType[typeId].name;
}

uSize Manager::typeSize(TypeId const& typeId) const { return this->mMetadataByType[typeId].size; }

IEVCSObject* Manager::createObject(TypeId const& typeId, Identifier const& netId)
{
	auto ptr = this->create(typeId);
	ptr->setNetId(netId);
	this->assignNetworkId(netId, ptr->id());
	return ptr;
}

IEVCSObject* Manager::getObject(TypeId const& typeId, Identifier const& netId)
{
	return this->get(typeId, this->getNetworkedObjectId(netId));
}

void Manager::destroyObject(TypeId const& typeId, Identifier const& netId)
{
	auto objectId = this->getNetworkedObjectId(netId);
	this->removeNetworkId(netId);
	this->destroy(typeId, objectId);
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

Component* Manager::create(ComponentTypeId const& typeId, bool bForceCreateNetId)
{
	uIndex objectId;
	void* pCompMem = this->mPoolByType[typeId].create(objectId);
	auto ptr = reinterpret_cast<Component*>(pCompMem);
	this->mMetadataByType[typeId].construct(ptr);
	ptr->setId(objectId);
	this->mAllocatedByType[typeId].insert(std::make_pair(objectId, ptr));

	auto const bReplicate = evcs::Core::Get()->shouldReplicate();
	if (bForceCreateNetId || bReplicate)
	{
		ptr->setNetId(this->nextNetworkId());
		this->assignNetworkId(ptr->netId(), ptr->id());
	}
	if (bReplicate)
	{
		auto pCreate = evcs::Core::Get()->replicateCreate();
		pCreate->setObjectEcsType(EType::eComponent)
			.setObjectTypeId(typeId)
			.setObjectNetId(ptr->netId());
		for (auto const& field : ptr->allFields())
		{
			pCreate->pushComponentField(
				field.first,
				(void*)(uSize(ptr) + field.first),
				field.second
			);
		}
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

	bool bWasReplicated = iter->second->isReplicated();
	auto netId = iter->second->netId();

	{
		auto iterOwner = this->mOwnerEntityByAllocatedId[typeId].find(id);
		if (iterOwner != this->mOwnerEntityByAllocatedId[typeId].end())
		{
			this->mOwnerEntityByAllocatedId[typeId].erase(iterOwner);
		}
	}
	this->mAllocatedByType[typeId].erase(iter);
	this->mPoolByType[typeId].destroy(id);

	if (bWasReplicated)
	{
		this->removeNetworkId(netId);
		if (evcs::Core::Get()->shouldReplicate())
		{
			evcs::Core::Get()->replicateDestroy()
				->setObjectEcsType(EType::eComponent)
				.setObjectTypeId(typeId)
				.setObjectNetId(netId)
				;
		}
	}

	evcs::Core::logger().log(
		LOG_VERBOSE, "Destroyed %s component %u with net-id(%u)",
		this->typeName(typeId).c_str(), id, netId
	);
}

void Manager::setComponentEntity(TypeId typeId, Identifier compId, Identifier entityId)
{
	this->mOwnerEntityByAllocatedId[typeId].insert(std::make_pair(compId, entityId));
}

std::optional<Identifier> Manager::getComponentEntityId(TypeId typeId, Identifier compId)
{
	auto iter = this->mOwnerEntityByAllocatedId[typeId].find(compId);
	if (iter != this->mOwnerEntityByAllocatedId[typeId].end())
	{
		return iter->second;
	}
	return std::nullopt;
}
