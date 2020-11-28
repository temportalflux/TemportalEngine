#include "ecs/view/ECViewManager.hpp"

using namespace ecs;
using namespace ecs::view;

std::shared_ptr<View> Manager::createView()
{
	this->mMutex.lock();

	uIndex objectId;
	auto shared = std::shared_ptr<View>(this->mPool.create(objectId), std::bind(&Manager::destroy, this, std::placeholders::_1));
	shared->mId = objectId;
	this->mAllocatedObjects.insert(std::make_pair(objectId, shared));

	this->mMutex.unlock();
	return shared;
}

void Manager::destroy(View *pCreated)
{
	this->mMutex.lock();

	auto allocatedIter = this->mAllocatedObjects.find(pCreated->mId);
	assert(allocatedIter != this->mAllocatedObjects.end());
	this->mAllocatedObjects.erase(allocatedIter);

	this->mPool.destroy(pCreated->mId);

	this->mMutex.unlock();
}

std::shared_ptr<View> Manager::get(Identifier const &id) const
{
	auto iter = this->mAllocatedObjects.find(id);
	return iter != this->mAllocatedObjects.end() ? iter->second.lock() : nullptr;
}
