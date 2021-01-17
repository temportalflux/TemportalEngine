#include "ecs/view/ECViewManager.hpp"

#include "ecs/Core.hpp"
#include "network/packet/NetworkPacketECSReplicate.hpp"

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

IEVCSObject* Manager::createObject(TypeId const& typeId)
{
	return this->create(typeId);
}

View* Manager::create(ViewTypeId const& typeId)
{
	this->mMutex.lock();

	auto& typeMeta = this->getTypeMetadata(typeId);
	
	uIndex objectId;
	auto ptr = this->mPool.create<View>(objectId);
	ptr->id = objectId;
	typeMeta.construct(ptr);

	uIndex idxRecord = this->mAllocatedObjects.insert(ViewRecord { typeId, objectId, ptr });
	this->mObjectsById[objectId] = ptr;

	if (typeMeta.mCount == 0) typeMeta.mFirstAllocatedIdx = idxRecord;
	typeMeta.mCount++;

	for (uIndex nextTypeId = typeId + 1; nextTypeId < ECS_MAX_VIEW_TYPE_COUNT; ++nextTypeId)
	{
		this->mRegisteredTypes[nextTypeId].mFirstAllocatedIdx++;
	}

	if (ecs::Core::Get()->shouldReplicate())
	{
		ptr->netId = this->nextNetworkId();
		ecs::Core::Get()->replicateCreate()
			->setObjectEcsType(ecs::EType::eView)
			.setObjectTypeId(typeId)
			.setObjectNetId(ptr->netId)
			;
	}

	this->mMutex.unlock();
	return ptr;
}

View* Manager::get(Identifier const& id)
{
	return this->mObjectsById[id];
}

void Manager::destroy(ViewTypeId const& typeId, Identifier const& id)
{
	this->mMutex.lock();

	auto idxRecord = this->mAllocatedObjects.search([typeId, id](ViewRecord const& record) -> i8
	{
		// typeId <=> record.typeId
		auto typeComp = typeId < record.typeId ? -1 : (typeId > record.typeId ? 1 : 0);
		if (typeComp != 0) return typeComp;
		// pCreated->mId <=> record.objectId
		auto objComp = id < record.objectId ? -1 : (id > record.objectId ? 1 : 0);
		return objComp;
	});
	assert(idxRecord);
	this->mAllocatedObjects.remove(*idxRecord);
	
	auto netId = this->mObjectsById[id]->netId;
	this->mObjectsById[id] = nullptr;

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

	if (ecs::Core::Get()->shouldReplicate())
	{
		ecs::Core::Get()->replicateDestroy()
			->setObjectEcsType(ecs::EType::eView)
			.setObjectTypeId(typeId)
			.setObjectNetId(netId)
			;
	}

	this->mPool.destroy<View>(id);

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

View* Manager::getNetworked(Identifier const& netId) const
{
	return this->mObjectsById[this->getNetworkedObjectId(netId)];
}
