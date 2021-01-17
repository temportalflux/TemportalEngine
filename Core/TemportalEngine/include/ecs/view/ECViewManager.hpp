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
		uIndex mFirstAllocatedIdx;
		uSize mCount;
		std::function<void(std::shared_ptr<View>)> initView;
	};

public:
	struct ViewIterator
	{
	public:
		ViewIterator(Manager *manager, uIndex idxRecord) : mpManager(manager), mIdxRecord(idxRecord) {}
		std::shared_ptr<View> operator*() { return this->mpManager->getRecord(mIdxRecord).ptr.lock(); }
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
	void registerType()
	{
		assert(sizeof(TView) == sizeof(View));
		assert(this->mRegisteredTypeCount < ECS_MAX_VIEW_TYPE_COUNT);
		TView::TypeId = this->mRegisteredTypeCount++;
		this->mRegisteredTypes[TView::TypeId] = TypeMetadata {
			0, 0, &TView::initView
		};
	}

	std::shared_ptr<View> create(ViewTypeId const& typeId);

	template <typename TView>
	std::shared_ptr<TView> create()
	{
		return std::reinterpret_pointer_cast<TView>(this->createView(TView::TypeId));
	}

	ViewIterable getAllOfType(ViewTypeId const &typeId);

	std::shared_ptr<View> getNetworked(Identifier const& netId) const;

private:
	thread::MutexLock mMutex;

	TypeMetadata mRegisteredTypes[ECS_MAX_VIEW_TYPE_COUNT];
	uSize mRegisteredTypeCount;

	ObjectPool mPool;

	struct ViewRecord
	{
		ViewTypeId typeId;
		Identifier objectId;
		std::weak_ptr<View> ptr;
		bool operator<(ViewRecord const& other) const;
		bool operator>(ViewRecord const& other) const;
	};
	FixedArray<ViewRecord, ECS_MAX_VIEW_COUNT> mAllocatedObjects;
	FixedArray<std::weak_ptr<View>, ECS_MAX_VIEW_COUNT> mObjectsById;
	
	TypeMetadata& getTypeMetadata(ViewTypeId const& id) { return this->mRegisteredTypes[id]; }
	ViewRecord& getRecord(uIndex const& idxRecord);

	void destroy(ViewTypeId const& typeId, View *pCreated);

};

NS_END
NS_END
