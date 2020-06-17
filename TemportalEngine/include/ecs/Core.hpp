#pragma once

#include "TemportalEnginePCH.hpp"

#include "ObjectPool.hpp"

#define ECS_MAX_ENTITY_COUNT 1024
#define ECS_MAX_COMPONENT_COUNT 1024
#define ECS_ENTITY_MAX_COMPONENT_COUNT 32
#define ECS_MAX_COMPONENT_TYPE_COUNT 16

NS_ECS

// TODO: Might be worth considering if guids are really worth it (they are 16 bytes a pop). Might be able to use ui16 instead (max of 65536)
typedef utility::Guid Identifer;

struct Entity
{
	Identifer id;
};

struct Component
{
	Identifer id;
};

struct ComponentTransform : public Component
{
	f32 x, y, z;

	ComponentTransform() : x(1), y(2), z(3) {}
};

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
	typedef uSize ComponentTypeId;

	Core()
	{
		this->mpComponentMemory = nullptr;
	}

	~Core()
	{
		if (this->mpComponentMemory != nullptr)
		{
			free(this->mpComponentMemory);
			this->mpComponentMemory = nullptr;
		}
	}

	template <typename TComponent>
	ComponentTypeId registerType()
	{
		ComponentTypeId typeId = this->mRegisteredTypeCount;
		this->mComponentMetadataByType[typeId] = {
			sizeof(TComponent),
			sizeof(ObjectPool<Identifer, TComponent, ECS_MAX_COMPONENT_COUNT>)
		};
		this->mRegisteredTypeCount++;
		return typeId;
	}

	void constructComponentPools()
	{
		uSize sumSizeOfAllPools = 0;
		for (uSize i = 0; i < this->mRegisteredTypeCount; ++i)
			sumSizeOfAllPools += this->mComponentMetadataByType[i].objectPoolSize;

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

	template <typename TComponent, typename... TArgs>
	TComponent* create(ComponentTypeId const &type, TArgs... args)
	{
		auto pool = this->lookupPool<TComponent>(type);
		if (pool == nullptr) return nullptr;
		auto id = utility::Guid::create();
		TComponent* ptr = pool->create(id, args...);
		((ecs::Component*)ptr)->id = id;
		return ptr;
	}

	template <typename TComponent>
	TComponent* lookup(ComponentTypeId const &type, utility::Guid const &componentId)
	{
		auto pool = this->lookupPool<TComponent>(type);
		if (pool == nullptr) return nullptr;
		return pool->lookup(componentId);
	}

private:

	ObjectPool<Identifer, Entity, ECS_MAX_ENTITY_COUNT> mEntities;

	ComponentTypeMetadata mComponentMetadataByType[ECS_MAX_COMPONENT_TYPE_COUNT];
	uSize mRegisteredTypeCount;

	void* mComponentPoolsByType[ECS_MAX_COMPONENT_TYPE_COUNT];

	/**
	 * Pointer to the giant chunk that holds all components.
	 * /Could/ have used `memory::MemoryChunk`, but we know exactly
	 * how big each portion should be and how much is needed
	 * ahead of time, so dynamic memory management is not needed here.
	 */
	void* mpComponentMemory;


	template <typename TComponent>
	ObjectPool<Identifer, TComponent, ECS_MAX_COMPONENT_COUNT>* lookupPool(ComponentTypeId const &type)
	{
		return reinterpret_cast<ObjectPool<Identifer, TComponent, ECS_MAX_COMPONENT_COUNT>*>(
			this->mComponentPoolsByType[type]
		);
	}


};

NS_END
