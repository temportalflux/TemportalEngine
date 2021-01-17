#include "ecs/entity/EntityManager.hpp"

using namespace ecs;

EntityManager::EntityManager()
{

}

EntityManager::~EntityManager()
{
	// Must not have any more entities lying about
	assert(this->mAllocatedObjects.size() <= 0);
}

std::shared_ptr<Entity> EntityManager::create()
{
	this->mMutex.lock();

	uIndex objectId;
	auto shared = std::shared_ptr<Entity>(this->mPool.create(objectId), std::bind(&EntityManager::destroy, this, std::placeholders::_1));
	shared->id = objectId;
	this->mAllocatedObjects.insert(std::make_pair(objectId, shared));

	this->mOwnedObjects.insert(std::make_pair(objectId, shared));
	
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

	// Actually release the memory
	this->mPool.destroy(pCreated->id);

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
