#pragma once

#include "TemportalEnginePCH.hpp"

#include "FixedSortedArray.hpp"
#include "ObjectPool.hpp"

#include "ecs/types.h"
#include "ecs/component/Component.hpp"
#include "thread/MutexLock.hpp"

FORWARD_DEF(NS_ECS, class Core);

NS_ECS

class ComponentManager
{
	typedef FixedSortedArray<Identifier, ECS_MAX_COMPONENT_COUNT> TAvailableIds;
	template <typename TComponent>
	using TPool = ObjectPool<Identifier, TComponent, ECS_MAX_COMPONENT_COUNT>;

	struct TypeMetadata
	{
		// The sizeof a given component for the type
		uSize size;
		// the sizeof an object pool for a component type
		uSize objectPoolSize;

		std::string name;
	};

public:
	ComponentManager(Core *pCore);
	~ComponentManager();

	template <typename TComponent>
	ComponentTypeId registerType(std::string const& name)
	{
		TComponent::TypeId = this->mRegisteredTypeCount;
		this->registerType(TComponent::TypeId, {
			sizeof(TComponent),
			sizeof(TPool<TComponent>),
			name
		});
		return TComponent::TypeId;
	}

	void allocatePools();

	template <typename TComponent>
	std::shared_ptr<TComponent> create()
	{
		auto pool = this->lookupPool<TComponent>(TComponent::TypeId);
		if (pool == nullptr) return nullptr;

		this->mMutex.lock();

		auto id = this->dequeueOrCreateId(TComponent::TypeId, pool);
		auto ptr = std::shared_ptr<TComponent>(
			pool->create(id),
			std::bind(&ComponentManager::destroy, this, std::placeholders::_1)
		);
		ptr->id = id;

		this->mMutex.unlock();
		return ptr;
	}

	template <typename TComponent>
	std::shared_ptr<TComponent> get(Identifier const &id)
	{
		auto pool = this->lookupPool<TComponent>(TComponent::TypeId);
		if (pool == nullptr) return nullptr;
		return pool->lookup(id);
	}

private:
	Core *mpCore;
	thread::MutexLock mMutex;

	TypeMetadata mMetadataByType[ECS_MAX_COMPONENT_TYPE_COUNT];
	uSize mRegisteredTypeCount;

	/**
	 * Pointer to the giant chunk that holds all components.
	 * /Could/ have used `memory::MemoryChunk`, but we know exactly
	 * how big each portion should be and how much is needed
	 * ahead of time, so dynamic memory management is not needed here.
	 */
	void* mpPoolMemory;

	void* mPoolByType[ECS_MAX_COMPONENT_TYPE_COUNT];
	TAvailableIds mAvailableIdsByType[ECS_MAX_COMPONENT_TYPE_COUNT];

	void registerType(ComponentTypeId const& id, TypeMetadata const& metadata);

	template <typename TComponent>
	TPool<TComponent>* lookupPool(ComponentTypeId const &type)
	{
		return reinterpret_cast<TPool<TComponent>*>(this->mPoolByType[type]);
	}

	template <typename TComponent>
	Identifier dequeueOrCreateId(ComponentTypeId const& typeId, TPool<TComponent> *pool)
	{
		auto& unusedIds = this->mAvailableIdsByType[typeId];
		return unusedIds.size() > 0 ? unusedIds.dequeue() : (Identifier)pool->size();
	}

	template <typename TComponent>
	void destroy(TComponent *pCreated)
	{
		this->mMutex.lock();
		auto pool = this->lookupPool<TComponent>(TComponent::TypeId);
		if (pool == nullptr) return;
		pool->destroy(TComponent->id);
		this->mAvailableIdsByType[TComponent::TypeId].insert(TComponent->id);
		this->mMutex.unlock();
	}

};

NS_END
