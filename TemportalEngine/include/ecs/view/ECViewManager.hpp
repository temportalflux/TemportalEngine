#pragma once

#include "TemportalEnginePCH.hpp"

#include "FixedSortedArray.hpp"
#include "ObjectPool.hpp"

#include "ecs/types.h"
#include "ecs/view/ECView.hpp"
#include "thread/MutexLock.hpp"

NS_ECS
NS_VIEW

class Manager
{
	typedef FixedSortedArray<Identifier, ECS_MAX_VIEW_COUNT> TAvailableIds;
	typedef ObjectPool<Identifier, View, ECS_MAX_VIEW_COUNT> TPool;
	typedef std::unordered_map<Identifier, std::weak_ptr<View>> TAllocatedObjectMap;

public:

	template <typename TView>
	void registerType()
	{
		TView::TypeId = this->mRegisteredTypeCount++;
	}

	template <typename TView>
	std::shared_ptr<TView> create()
	{
		// All views have the same memory size, they just have different type names according to what systems they support and what components they slot.
		std::shared_ptr<TView> shared = std::reinterpret_pointer_cast<TView>(createView());
		// They DO Need to be properly constructed though, so this makes sure that the view object is constructed using the default constructor of the desired type
		new (shared.get()) TView();
		return shared;
	}

	std::shared_ptr<View> get(Identifier const &id) const;

private:
	thread::MutexLock mMutex;

	uSize mRegisteredTypeCount;

	TAvailableIds mAvailableIds;
	TPool mPool;
	TAllocatedObjectMap mAllocatedObjects;

	std::shared_ptr<View> createView();
	Identifier dequeueOrCreateId();
	void destroy(View *pCreated);

};

NS_END
NS_END
