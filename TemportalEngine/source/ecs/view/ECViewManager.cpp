#include "ecs/view/ECViewManager.hpp"

using namespace ecs;
using namespace ecs::view;

std::shared_ptr<View> Manager::createView()
{
	this->mMutex.lock();

	auto id = this->dequeueOrCreateId();
	auto shared = std::shared_ptr<View>(this->mPool.create(id), std::bind(&Manager::destroy, this, std::placeholders::_1));
	shared->mId = id;
	this->mAllocatedObjects.insert(std::make_pair(id, shared));

	this->mMutex.unlock();
	return shared;
}

Identifier Manager::dequeueOrCreateId()
{
	return this->mAvailableIds.size() > 0 ? this->mAvailableIds.dequeue() : (Identifier)this->mAllocatedObjects.size();
}

void Manager::destroy(View *pCreated)
{
	this->mMutex.lock();

	auto allocatedIter = this->mAllocatedObjects.find(pCreated->mId);
	assert(allocatedIter != this->mAllocatedObjects.end());

	this->mAvailableIds.insert(pCreated->mId);
	this->mAllocatedObjects.erase(allocatedIter);
	this->mPool.destroy(pCreated->mId);

	this->mMutex.unlock();
}

std::shared_ptr<View> Manager::get(Identifier const &id) const
{
	auto iter = this->mAllocatedObjects.find(id);
	return iter != this->mAllocatedObjects.end() ? iter->second.lock() : nullptr;
}
