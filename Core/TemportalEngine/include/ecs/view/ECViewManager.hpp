#pragma once

#include "TemportalEnginePCH.hpp"

#include "dataStructures/FixedArray.hpp"
#include "dataStructures/ObjectPool.hpp"

#include "ecs/types.h"
#include "ecs/view/ECView.hpp"
#include "thread/MutexLock.hpp"

NS_ECS
NS_VIEW

class Manager
{
	typedef FixedArray<Identifier, ECS_MAX_VIEW_COUNT> TAvailableIds;
	typedef ObjectPool<View, ECS_MAX_VIEW_COUNT> TPool;

	struct TypeMetadata
	{
		uIndex mFirstAllocatedIdx;
		uSize mCount;
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

	template <typename TView>
	void registerType()
	{
		assert(sizeof(TView) == sizeof(View));
		assert(this->mRegisteredTypeCount < ECS_MAX_VIEW_TYPE_COUNT);
		TView::TypeId = this->mRegisteredTypeCount++;
		this->mRegisteredTypes[TView::TypeId] = TypeMetadata{ 0, 0 };
	}

	template <typename TView>
	std::shared_ptr<TView> create()
	{
		// All views have the same memory size, they just have different type names according to what systems they support and what components they slot.
		std::shared_ptr<TView> shared = std::reinterpret_pointer_cast<TView>(createView(TView::TypeId));
		// They DO Need to be properly constructed though, so this makes sure that the view object is constructed using the default constructor of the desired type
		new (shared.get()) TView();
		return shared;
	}

	ViewIterable getAllOfType(ViewTypeId const &typeId);

private:
	thread::MutexLock mMutex;

	TypeMetadata mRegisteredTypes[ECS_MAX_VIEW_TYPE_COUNT];
	uSize mRegisteredTypeCount;

	TPool mPool;

	struct ViewRecord
	{
		ViewTypeId typeId;
		Identifier objectId;
		std::weak_ptr<View> ptr;
		bool operator<(ViewRecord const& other) const;
		bool operator>(ViewRecord const& other) const;
	};
	FixedArray<ViewRecord, ECS_MAX_VIEW_COUNT> mAllocatedObjects;
	
	TypeMetadata& getTypeMetadata(ViewTypeId const& id) { return this->mRegisteredTypes[id]; }
	ViewRecord& getRecord(uIndex const& idxRecord);

	std::shared_ptr<View> createView(ViewTypeId const& typeId);
	void destroy(ViewTypeId const& typeId, View *pCreated);

};

NS_END
NS_END
