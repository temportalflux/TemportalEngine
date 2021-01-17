#pragma once

#include "ecs/ECSNetworkedManager.hpp"

#include "dataStructures/FixedArray.hpp"
#include "dataStructures/ObjectPool.hpp"
#include "ecs/component/Component.hpp"
#include "thread/MutexLock.hpp"

FORWARD_DEF(NS_ECS, class Core);

NS_ECS
NS_COMPONENT

class Manager : public ecs::NetworkedManager
{

	struct TypeMetadata
	{
		// The sizeof a given component for the type
		uSize size;
		uSize objectCount;
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
			TComponent::MaxPoolSize,
			name
		});
		return TComponent::TypeId;
	}

	void allocatePools();

	std::shared_ptr<Component> create(ComponentTypeId const& typeId);
	std::shared_ptr<Component> get(ComponentTypeId const& typeId, Identifier const& id);

	template <typename TComponent>
	std::shared_ptr<TComponent> create()
	{
		return std::reinterpret_pointer_cast<TComponent>(this->create(TComponent::TypeId));
	}

	template <typename TComponent>
	std::shared_ptr<TComponent> get(Identifier const &id)
	{
		return std::reinterpret_pointer_cast<TComponent>(this->get(TComponent::TypeId), id);
	}

	std::shared_ptr<Component> getNetworked(ComponentTypeId const& typeId, Identifier const& netId);

private:
	Core *mpCore;

	TypeMetadata mMetadataByType[ECS_MAX_COMPONENT_TYPE_COUNT];
	uSize mRegisteredTypeCount;

	/**
	 * Pointer to the giant chunk that holds all components.
	 * /Could/ have used `memory::MemoryChunk`, but we know exactly
	 * how big each portion should be and how much is needed
	 * ahead of time, so dynamic memory management is not needed here.
	 */
	void* mpPoolMemory;
	ObjectPool mPoolByType[ECS_MAX_COMPONENT_TYPE_COUNT];
	std::map<Identifier, std::weak_ptr<Component>> mAllocatedByType[ECS_MAX_COMPONENT_TYPE_COUNT];

	void registerType(ComponentTypeId const& id, TypeMetadata const& metadata);
	void destroy(ComponentTypeId const& typeId, Component *pCreated);

};

NS_END
NS_END
