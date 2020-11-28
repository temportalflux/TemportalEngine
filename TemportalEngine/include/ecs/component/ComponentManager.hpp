#pragma once

#include "TemportalEnginePCH.hpp"

#include "FixedSortedArray.hpp"
#include "ObjectPool.hpp"

#include "ecs/types.h"
#include "ecs/component/Component.hpp"
#include "thread/MutexLock.hpp"

FORWARD_DEF(NS_ECS, class Core);

NS_ECS
NS_COMPONENT

class Manager
{
	template <typename TComponent>
	using TAvailableIds = FixedSortedArray<Identifier, TComponent::MaxPoolSize>;
	template <typename TComponent>
	using TPool = ObjectPool<TComponent, TComponent::MaxPoolSize>;

	struct TypeMetadata
	{
		// The sizeof a given component for the type
		uSize size;
		// the sizeof an object pool for a component type
		uSize objectPoolSize;
		uSize availableIdArraySize;

		std::string name;
	};

public:
	Manager(Core *pCore);
	~Manager();

	template <typename TComponent>
	ComponentTypeId registerType(std::string const& name)
	{
		TComponent::TypeId = this->mRegisteredTypeCount;
		this->registerType(TComponent::TypeId, {
			sizeof(TComponent),
			sizeof(TPool<TComponent>),
			sizeof(TAvailableIds<TComponent>),
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

		uIndex objectId;
		auto ptr = std::shared_ptr<TComponent>(
			pool->create(objectId),
			std::bind(&Manager::destroy<TComponent>, this, std::placeholders::_1)
		);
		ptr->id = objectId;

		this->mMutex.unlock();
		return ptr;
	}

	template <typename TComponent>
	TComponent* get(Identifier const &id)
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
	void* mpAvailableIdMemory;

	void* mPoolByType[ECS_MAX_COMPONENT_TYPE_COUNT];
	void* mAvailableIdsByType[ECS_MAX_COMPONENT_TYPE_COUNT];

	void registerType(ComponentTypeId const& id, TypeMetadata const& metadata);

	template <typename TComponent>
	TPool<TComponent>* lookupPool(ComponentTypeId const &type)
	{
		return reinterpret_cast<TPool<TComponent>*>(this->mPoolByType[type]);
	}

	template <typename TComponent>
	Identifier dequeueOrCreateId(ComponentTypeId const& typeId, TPool<TComponent> *pool)
	{
		auto* unusedIds = reinterpret_cast<TAvailableIds<TComponent>*>(this->mAvailableIdsByType[typeId]);
		return unusedIds->size() > 0 ? unusedIds->dequeue() : (Identifier)pool->size();
	}

	template <typename TComponent>
	void destroy(TComponent *pCreated)
	{
		this->mMutex.lock();
		
		auto pool = this->lookupPool<TComponent>(TComponent::TypeId);
		if (pool == nullptr) return;

		auto* unusedIds = reinterpret_cast<TAvailableIds<TComponent>*>(this->mAvailableIdsByType[TComponent::TypeId]);
		unusedIds->insert(pCreated->id);
		pool->destroy(pCreated->id);
		
		this->mMutex.unlock();
	}

};

NS_END
NS_END
