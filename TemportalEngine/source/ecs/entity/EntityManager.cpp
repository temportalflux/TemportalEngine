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

	auto id = this->dequeueOrCreateId();
	auto shared = std::shared_ptr<Entity>(this->mPool.create(id), std::bind(&EntityManager::destroy, this, std::placeholders::_1));
	shared->id = id;
	this->mAllocatedObjects.insert(std::make_pair(id, shared));
	
	this->mMutex.unlock();
	return shared;
}

Identifier EntityManager::dequeueOrCreateId()
{
	return this->mAvailableIds.size() > 0 ? this->mAvailableIds.dequeue() : (Identifier)this->mAllocatedObjects.size();
}

void EntityManager::destroy(Entity *pCreated)
{
	this->mMutex.lock();

	auto allocatedIter = this->mAllocatedObjects.find(pCreated->id);
	assert(allocatedIter != this->mAllocatedObjects.end());

	this->mAvailableIds.insert(pCreated->id);
	this->mAllocatedObjects.erase(allocatedIter);
	this->mPool.destroy(pCreated->id);

	this->mMutex.unlock();
}

std::shared_ptr<Entity> EntityManager::get(Identifier const &id) const
{
	auto iter = this->mAllocatedObjects.find(id);
	return iter != this->mAllocatedObjects.end() ? iter->second.lock() : nullptr;
}
