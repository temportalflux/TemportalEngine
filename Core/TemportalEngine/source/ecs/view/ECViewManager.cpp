#include "ecs/view/ECViewManager.hpp"

using namespace ecs;
using namespace ecs::view;

bool Manager::ViewRecord::operator<(Manager::ViewRecord const& other) const
{
	return typeId < other.typeId && objectId < other.objectId;
}

bool Manager::ViewRecord::operator>(Manager::ViewRecord const& other) const
{
	return typeId > other.typeId && objectId > other.objectId;
}

std::shared_ptr<View> Manager::createView(ViewTypeId const& typeId)
{
	this->mMutex.lock();

	uIndex objectId;
	auto shared = std::shared_ptr<View>(
		this->mPool.create(objectId),
		std::bind(&Manager::destroy, this, typeId, std::placeholders::_1)
	);
	shared->mId = objectId;
	uIndex idxRecord = this->mAllocatedObjects.insert(ViewRecord { typeId, objectId, std::weak_ptr(shared) });

	auto& typeMeta = this->getTypeMetadata(typeId);
	if (typeMeta.mCount == 0 || idxRecord < typeMeta.mFirstAllocatedIdx)
	{
		typeMeta.mFirstAllocatedIdx = idxRecord;
	}
	typeMeta.mCount++;

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
		auto objComp = pCreated->mId < record.objectId ? -1 : (pCreated->mId > record.objectId ? 1 : 0);
		return objComp;
	});
	assert(idxRecord);
	this->mAllocatedObjects.remove(*idxRecord);

	auto& typeMeta = this->getTypeMetadata(typeId);
	typeMeta.mCount--;
	if (typeMeta.mFirstAllocatedIdx == *idxRecord)
	{
		typeMeta.mFirstAllocatedIdx = typeMeta.mCount > 0 ? *idxRecord + 1 : 0;
	}

	this->mPool.destroy(pCreated->mId);

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
