#pragma once

#include "ecs/ECSNetworkedManager.hpp"

#include "dataStructures/FixedArray.hpp"
#include "dataStructures/ObjectPool.hpp"
#include "ecs/view/ECView.hpp"
#include "thread/MutexLock.hpp"

NS_ECS
NS_VIEW

class Manager : public ecs::NetworkedManager
{
	typedef FixedArray<Identifier, ECS_MAX_VIEW_COUNT> TAvailableIds;

	struct TypeMetadata
	{
		std::string name;
		std::function<void(View* pView)> construct;
		uIndex mFirstAllocatedIdx;
		uSize mCount;
	};

public:
	struct ViewIterator
	{
	public:
		ViewIterator(Manager *manager, uIndex idxRecord) : mpManager(manager), mIdxRecord(idxRecord) {}
		View* operator*() { return this->mpManager->getRecord(mIdxRecord).ptr; }
		void operator++() { this->mIdxRecord++; }
		bool operator!=(ViewIterator const& other) { return this->mIdxRecord != other.mIdxRecord; }
	private:
		Manager *mpManager;
		ViewTypeId mIdxRecord;
	};
	struct ViewIterable
	{
		ViewIterable(Manager *manager, uIndex recordStart, uSize recordCount) : mpManager(manager), mIdxStart(recordStart), mCount(recordCount) {}
		ViewIterator begin() { return ViewIterator(mpManager, mIdxStart); }
		ViewIterator end() { return ViewIterator(mpManager, mIdxStart + mCount); }
	private:
		Manager *mpManager;
		uIndex mIdxStart;
		uSize mCount;
	};

	Manager();

	template <typename TView>
	void registerType(std::string const& name)
	{
		assert(sizeof(TView) == sizeof(View));
		assert(this->mRegisteredTypeCount < ECS_MAX_VIEW_TYPE_COUNT);
		TView::TypeId = this->mRegisteredTypeCount++;
		this->mRegisteredTypes[TView::TypeId] = TypeMetadata {
			name, &TView::construct, 0, 0
		};
	}

	std::string typeName(TypeId const& typeId) const override;
	IEVCSObject* createObject(TypeId const& typeId, Identifier const& netId) override;
	IEVCSObject* getObject(TypeId const& typeId, Identifier const& netId) override;
	void destroyObject(TypeId const& typeId, Identifier const& netId) override;

	View* create(ViewTypeId const& typeId);
	View* get(Identifier const& id);
	void destroy(ViewTypeId const& typeId, Identifier const& id);

	template <typename TView>
	TView* create()
	{
		return dynamic_cast<TView*>(this->create(TView::TypeId));
	}

	ViewIterable getAllOfType(ViewTypeId const &typeId);

private:
	thread::MutexLock mMutex;

	TypeMetadata mRegisteredTypes[ECS_MAX_VIEW_TYPE_COUNT];
	uSize mRegisteredTypeCount;

	ObjectPool mPool;

	struct ViewRecord
	{
		ViewTypeId typeId;
		Identifier objectId;
		View* ptr;
		bool operator<(ViewRecord const& other) const;
		bool operator>(ViewRecord const& other) const;
	};
	FixedArray<ViewRecord, ECS_MAX_VIEW_COUNT> mAllocatedObjects;
	FixedArray<View*, ECS_MAX_VIEW_COUNT> mObjectsById;
	
	TypeMetadata& getTypeMetadata(ViewTypeId const& id) { return this->mRegisteredTypes[id]; }
	ViewRecord& getRecord(uIndex const& idxRecord);

};

NS_END
NS_END
