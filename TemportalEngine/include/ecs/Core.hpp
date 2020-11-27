#pragma once

#include "TemportalEnginePCH.hpp"

#include "FixedSortedArray.hpp"
#include "ObjectPool.hpp"
#include "ecs/types.h"
#include "ecs/Entity.hpp"
#include "ecs/component/Component.hpp"
#include "logging/Logger.hpp"

NS_ECS

struct ComponentTypeMetadata
{
	// The size of a given component for the type
	uSize size;
	uSize objectPoolSize;
};

class Core
{
	template <typename TValue>
	using TComponentHashMap = FixedHashMap<std::string, TValue, ECS_MAX_COMPONENT_TYPE_COUNT>;

public:

	Core()
	{
		this->mpComponentMemory = nullptr;
		this->mRegisteredTypeCount = 0;
	}

	~Core()
	{
		// Must not have any more entities lying about
		assert(this->mEntities.size() <= 0);

		if (this->mpComponentMemory != nullptr)
		{
			free(this->mpComponentMemory);
			this->mpComponentMemory = nullptr;
		}
	}

	Core& setLog(logging::Logger log)
	{
		this->mLog = log;
		return *this;
	}

	template <typename... TArgs>
	void log(logging::ECategory category, logging::Message format, TArgs... args)
	{
		this->mLog.log(category, format, args...);
	}

	template <typename TComponent, uSize Capacity>
	ComponentTypeId registerType(char const *name)
	{
		TComponent::TypeId = this->mRegisteredTypeCount;
		this->mComponentMetadataByType[TComponent::TypeId] = {
			sizeof(TComponent),
			sizeof(ObjectPool<Identifier, TComponent, Capacity>)
		};
		this->mRegisteredTypeCount++;
		this->log(LOG_INFO, "Registered ECS component type %s with id %i and size %i",
			name, TComponent::TypeId, this->mComponentMetadataByType[TComponent::TypeId].size
		);
		return TComponent::TypeId;
	}

	void constructComponentPools()
	{
		uSize sumSizeOfAllPools = 0;
		for (uSize i = 0; i < this->mRegisteredTypeCount; ++i)
		{
			sumSizeOfAllPools += this->mComponentMetadataByType[i].objectPoolSize;
		}

		this->log(LOG_INFO, "Allocating %i bytes for object pools", sumSizeOfAllPools);

		// this memory chunk should now have enough room for:
		// 1) memory manager header
		// 2) some number of object pools equivalent to the number of types registered
		this->mpComponentMemory = malloc(sumSizeOfAllPools);

		// NOTE: Assumes each pool can be initialized by clearing its memory to 0
		memset(this->mpComponentMemory, 0, sumSizeOfAllPools);

		uSize nextPoolPtrLoc = (uSize)this->mpComponentMemory;
		for (ComponentTypeId typeId = 0; typeId < this->mRegisteredTypeCount; ++typeId)
		{
			this->mComponentPoolsByType[typeId] = (void*)nextPoolPtrLoc;
			nextPoolPtrLoc += this->mComponentMetadataByType[typeId].objectPoolSize;
		}
	}

	std::shared_ptr<Entity> createEntity()
	{
		auto id = this->nextEntityId();
		auto shared = std::shared_ptr<Entity>(this->mEntities.create(id), std::bind(&Core::destroyEntity, this, std::placeholders::_1));
		shared->id = id;
		this->mEntityPtrs.insert(std::make_pair(id, shared));
		return shared;
	}

	std::shared_ptr<Entity> getEntity(Identifier const &id)
	{
		auto iter = this->mEntityPtrs.find(id);
		return iter != this->mEntityPtrs.end() ? iter->second.lock() : nullptr;
	}

	// TODO: Core should lock everything about the component type when creating a new component
	template <typename TComponent, typename... TArgs>
	TComponent* create(TArgs... args)
	{
		ComponentTypeId const type = TComponent::TypeId;
		auto pool = this->lookupPool<TComponent>(type);
		if (pool == nullptr) return nullptr;
		auto id = this->nextId(type, pool);
		TComponent* ptr = pool->create(id, args...);
		ptr->id = id;
		return ptr;
	}

	template <typename TComponent>
	TComponent* lookup(Identifier const &componentId)
	{
		ComponentTypeId const type = TComponent::TypeId;
		auto pool = this->lookupPool<TComponent>(type);
		if (pool == nullptr) return nullptr;
		return pool->lookup(componentId);
	}

	// TODO: Core should lock everything about the component type when destroying a component
	template <typename TComponent>
	void destroyComponent(Identifier const &id)
	{
		ComponentTypeId const type = TComponent::TypeId;
		auto pool = this->lookupPool<TComponent>(type);
		if (pool == nullptr) return;
		pool->destroy(id);
		this->mUnusedComponentIds[type].insert(id);
	}

private:
	logging::Logger mLog;

	typedef ObjectPool<Identifier, Entity, ECS_MAX_ENTITY_COUNT> EntityPool;
	FixedSortedArray<Identifier, ECS_MAX_COMPONENT_COUNT> mUnusedEntityIds;
	EntityPool mEntities;
	std::unordered_map<Identifier, std::weak_ptr<Entity>> mEntityPtrs;

	ComponentTypeMetadata mComponentMetadataByType[ECS_MAX_COMPONENT_TYPE_COUNT];
	uSize mRegisteredTypeCount;

	FixedSortedArray<Identifier, ECS_MAX_COMPONENT_COUNT> mUnusedComponentIds[ECS_MAX_COMPONENT_TYPE_COUNT];
	void* mComponentPoolsByType[ECS_MAX_COMPONENT_TYPE_COUNT];

	/**
	 * Pointer to the giant chunk that holds all components.
	 * /Could/ have used `memory::MemoryChunk`, but we know exactly
	 * how big each portion should be and how much is needed
	 * ahead of time, so dynamic memory management is not needed here.
	 */
	void* mpComponentMemory;

	Identifier nextEntityId()
	{
		return this->mUnusedEntityIds.size() > 0 ? this->mUnusedEntityIds.dequeue() : this->createEntityId();
	}

	Identifier createEntityId()
	{
		return (Identifier)this->mEntities.size();
	}

	void destroyEntity(Entity* p)
	{
		// TODO: This should be threadsafe
		this->mUnusedEntityIds.insert(p->id);
		this->mEntityPtrs.erase(this->mEntityPtrs.find(p->id));
		this->mEntities.destroy(p->id);
	}

	template <typename TComponent>
	ObjectPool<Identifier, TComponent, ECS_MAX_COMPONENT_COUNT>* lookupPool(ComponentTypeId const &type)
	{
		return reinterpret_cast<ObjectPool<Identifier, TComponent, ECS_MAX_COMPONENT_COUNT>*>(
			this->mComponentPoolsByType[type]
		);
	}

	template <typename TComponent>
	Identifier nextId(ComponentTypeId const &type, ObjectPool<Identifier, TComponent, ECS_MAX_COMPONENT_COUNT>* pool)
	{
		auto& unusedIds = this->mUnusedComponentIds[type];
		return unusedIds.size() > 0 ? unusedIds.dequeue() : this->createId(pool);
	}

	template <typename TComponent>
	Identifier createId(ObjectPool<Identifier, TComponent, ECS_MAX_COMPONENT_COUNT>* pool)
	{
		return (Identifier)pool->size();
	}

};

NS_END
