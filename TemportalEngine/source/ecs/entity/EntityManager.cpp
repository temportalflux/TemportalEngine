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
	
	this->mMutex.unlock();
	return shared;
}

void EntityManager::destroy(Entity *pCreated)
{
	this->mMutex.lock();

	auto allocatedIter = this->mAllocatedObjects.find(pCreated->id);
	assert(allocatedIter != this->mAllocatedObjects.end());
	this->mAllocatedObjects.erase(allocatedIter);

	this->mPool.destroy(pCreated->id);

	this->mMutex.unlock();
}

std::shared_ptr<Entity> EntityManager::get(Identifier const &id) const
{
	auto iter = this->mAllocatedObjects.find(id);
	return iter != this->mAllocatedObjects.end() ? iter->second.lock() : nullptr;
}
