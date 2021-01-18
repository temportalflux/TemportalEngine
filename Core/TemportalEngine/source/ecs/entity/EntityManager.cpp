#include "ecs/entity/EntityManager.hpp"

#include "ecs/Core.hpp"
#include "network/packet/NetworkPacketECSReplicate.hpp"

using namespace ecs;

EntityManager::EntityManager()
	: mPool(sizeof(Entity), ECS_MAX_ENTITY_COUNT)
{
	// pools automatically free dynamic memory when they go out of scope
	this->mPool.allocateMemory();
}

EntityManager::~EntityManager()
{
	// Must not have any more entities lying about
	assert(this->mOwnedObjects.size() <= 0);
	assert(this->mAllocatedObjects.size() <= 0);
}

std::string EntityManager::typeName(TypeId const& typeId) const { return ""; }

IEVCSObject* EntityManager::createObject(TypeId const& typeId, Identifier const& netId)
{
	// ignores type id because its mostly for same-interface compatibility with views and components
	// returns the underlying pointer because the manager owns the object
	auto ptr = this->create().get();
	this->assignNetworkId(netId, ptr->id);
	ptr->netId = netId;
	return ptr;
}

IEVCSObject* EntityManager::getObject(TypeId const& typeId, Identifier const& netId)
{
	return this->get(this->getNetworkedObjectId(netId)).get();
}

void EntityManager::destroyObject(TypeId const& typeId, Identifier const& netId)
{
	auto objectId = this->getNetworkedObjectId(netId);
	std::string externalError = "The object will be removed from the network, but will still be alive due to external references.";
	if (!this->mOwnedObjects[objectId])
	{
		std::string err = "Attempting to destroy entity by replication, but the object is not owned by the manager. ";
		ecs::Core::logger().log(LOG_ERR, (err + externalError).c_str());
		this->removeNetworkId(netId);
		return;
	}
	if (!this->mOwnedObjects[objectId].unique())
	{
		std::string err = "Attempting to destroy entity by replication, but the object has %u external references. ";
		ecs::Core::logger().log(
			LOG_ERR, (err + externalError).c_str(),
			this->mOwnedObjects[objectId].use_count()
		);
	}
	this->release(objectId);
}

std::shared_ptr<Entity> EntityManager::create()
{
	this->mMutex.lock();

	uIndex objectId;
	auto shared = std::shared_ptr<Entity>(
		this->mPool.create<Entity>(objectId),
		std::bind(&EntityManager::destroy, this, std::placeholders::_1)
	);
	shared->id = objectId;
	shared->netId = 0;
	this->mAllocatedObjects.insert(std::make_pair(objectId, shared));
	this->mOwnedObjects.insert(std::make_pair(objectId, shared));
	
	if (ecs::Core::Get()->shouldReplicate())
	{
		shared->netId = this->nextNetworkId();
		this->assignNetworkId(shared->netId, shared->id);
		ecs::Core::Get()->replicateCreate()
			->setObjectEcsType(ecs::EType::eEntity)
			.setObjectNetId(shared->netId)
			;
	}

	this->mMutex.unlock();
	return shared;
}

void EntityManager::destroy(Entity *pCreated)
{
	this->mMutex.lock();

	auto id = pCreated->id;
	auto netId = pCreated->netId;

	auto ownedIter = this->mOwnedObjects.find(id);
	assert(ownedIter != this->mOwnedObjects.end());
	assert(!ownedIter->second); // ensure the ptr has already been released
	auto allocatedIter = this->mAllocatedObjects.find(id);
	assert(allocatedIter != this->mAllocatedObjects.end());

	// The order of replication vs memory deletion matters here.
	// First, we queue up the replication packet to tell receives the entity has been destroyed.
	// Second, we actually destroy the memory, causing components and views to also create their destroy packets.
	// If this was reversed, destroy packets for views and components would be first.

	if (this->hasNetworkId(netId))
	{
		this->removeNetworkId(netId);
	}
	if (ecs::Core::Get()->shouldReplicate())
	{
		ecs::Core::Get()->replicateDestroy()
			->setObjectEcsType(ecs::EType::eEntity)
			.setObjectNetId(netId)
			;
	}

	// This is safe because `EntityManager#destroy` is only called when there are no more references.
	// In order for that to be true, `mOwnedObjects[id]` must be invalid
	// (the user has called `Entity#kill` or `EntityManager#release`).
	this->mOwnedObjects.erase(ownedIter);

	// Actually release all references (allocated object map is used for lookup by id)
	this->mAllocatedObjects.erase(allocatedIter);

	// Actually release the memory
	this->mPool.destroy<Entity>(id);

	ecs::Core::logger().log(
		LOG_VERBOSE, "Destroyed entity %u with net-id(%u)",
		id, netId
	);

	this->mMutex.unlock();
}

std::shared_ptr<Entity> EntityManager::get(Identifier const &id) const
{
	auto iter = this->mAllocatedObjects.find(id);
	return iter != this->mAllocatedObjects.end() ? iter->second.lock() : nullptr;
}

void EntityManager::release(Identifier const& id)
{
	auto iter = this->mOwnedObjects.find(id);
	assert(iter != this->mOwnedObjects.end());
	// May cause the shared_ptr to be deleted, calling `EntityManager#destroy`
	iter->second.reset();
}

void EntityManager::releaseAll()
{
	for (auto& [id, ptr] : this->mOwnedObjects)
	{
		ptr.reset();
	}
}
