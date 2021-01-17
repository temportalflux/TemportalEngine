#include "ecs/view/ECViewManager.hpp"

using namespace ecs;
using namespace ecs::view;

bool Manager::ViewRecord::operator<(Manager::ViewRecord const& other) const
{
	if (typeId < other.typeId) return true;
	if (typeId == other.typeId) return objectId < other.objectId;
	return false;
}

bool Manager::ViewRecord::operator>(Manager::ViewRecord const& other) const
{
	if (typeId > other.typeId) return true;
	if (typeId == other.typeId) return objectId > other.objectId;
	return false;
}

Manager::Manager()
	: mPool(sizeof(View), ECS_MAX_VIEW_COUNT)
{
}

std::shared_ptr<View> Manager::create(ViewTypeId const& typeId)
{
	this->mMutex.lock();

	auto& typeMeta = this->getTypeMetadata(typeId);
	
	uIndex objectId;
	auto shared = std::shared_ptr<View>(
		this->mPool.create<View>(objectId),
		std::bind(&Manager::destroy, this, typeId, std::placeholders::_1)
	);
	shared->id = objectId;
	typeMeta.initView(shared);

	uIndex idxRecord = this->mAllocatedObjects.insert(ViewRecord { typeId, objectId, std::weak_ptr(shared) });
	this->mObjectsById[objectId] = shared;

	if (typeMeta.mCount == 0) typeMeta.mFirstAllocatedIdx = idxRecord;
	typeMeta.mCount++;

	for (uIndex nextTypeId = typeId + 1; nextTypeId < ECS_MAX_VIEW_TYPE_COUNT; ++nextTypeId)
	{
		this->mRegisteredTypes[nextTypeId].mFirstAllocatedIdx++;
	}

	this->mMutex.unlock();
	return shared;
}

void Manager::destroy(ViewTypeId const& typeId, View *pCreated)
{
	this->mMutex.lock();

	auto idxRecord = this->mAllocatedObjects.search([typeId, pCreated](ViewRecord const& record) -> i8
	{
		// typeId <=> record.typeId
		auto typeComp = typeId < record.typeId ? -1 : (typeId > record.typeId ? 1 : 0);
		if (typeComp != 0) return typeComp;
		// pCreated->mId <=> record.objectId
		auto objComp = pCreated->id < record.objectId ? -1 : (pCreated->id > record.objectId ? 1 : 0);
		return objComp;
	});
	assert(idxRecord);
	this->mAllocatedObjects.remove(*idxRecord);
	this->mObjectsById[pCreated->id].reset();

	auto& typeMeta = this->getTypeMetadata(typeId);
	typeMeta.mCount--;
	if (typeMeta.mFirstAllocatedIdx == *idxRecord)
	{
		typeMeta.mFirstAllocatedIdx = typeMeta.mCount > 0 ? *idxRecord + 1 : 0;
	}

	for (uIndex nextTypeId = typeId + 1; nextTypeId < ECS_MAX_VIEW_TYPE_COUNT; ++nextTypeId)
	{
		this->mRegisteredTypes[nextTypeId].mFirstAllocatedIdx--;
	}

	this->mPool.destroy<View>(pCreated->id);

	this->mMutex.unlock();
}

Manager::ViewIterable Manager::getAllOfType(ViewTypeId const &typeId)
{
	auto& typeMeta = this->getTypeMetadata(typeId);
	return Manager::ViewIterable(this, typeMeta.mFirstAllocatedIdx, typeMeta.mCount);
}

Manager::ViewRecord& Manager::getRecord(uIndex const& idxRecord)
{
	return this->mAllocatedObjects[idxRecord];
}

std::shared_ptr<View> Manager::getNetworked(Identifier const& netId) const
{
	auto weak = this->mObjectsById[this->getNetworkedObjectId(netId)];
	assert(!weak.expired());
	return weak.lock();
}
