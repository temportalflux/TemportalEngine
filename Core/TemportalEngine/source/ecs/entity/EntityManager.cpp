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

IEVCSObject* EntityManager::createObject(TypeId const& typeId)
{
	// ignores type id because its mostly for same-interface compatibility with views and components
	// returns the underlying pointer because the manager owns the object
	return this->create().get();
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

	// This is safe because `EntityManager#destroy` is only called when there are no more references.
	// In order for that to be true, `mOwnedObjects[id]` must be invalid
	// (the user has called `Entity#kill` or `EntityManager#release`).
	auto ownedIter = this->mOwnedObjects.find(pCreated->id);
	assert(ownedIter != this->mOwnedObjects.end());
	this->mOwnedObjects.erase(ownedIter);
	
	// Actually release all references (allocated object map is used for lookup by id)
	auto allocatedIter = this->mAllocatedObjects.find(pCreated->id);
	assert(allocatedIter != this->mAllocatedObjects.end());
	this->mAllocatedObjects.erase(allocatedIter);

	if (ecs::Core::Get()->shouldReplicate())
	{
		ecs::Core::Get()->replicateDestroy()
			->setObjectEcsType(ecs::EType::eEntity)
			.setObjectNetId(pCreated->netId)
			;
	}

	// Actually release the memory
	this->mPool.destroy<Entity>(pCreated->id);

	this->mMutex.unlock();
}

std::shared_ptr<Entity> EntityManager::get(Identifier const &id) const
{
	auto iter = this->mAllocatedObjects.find(id);
	return iter != this->mAllocatedObjects.end() ? iter->second.lock() : nullptr;
}

std::shared_ptr<Entity> EntityManager::getNetworked(Identifier const& netId) const
{
	return this->get(this->getNetworkedObjectId(netId));
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
