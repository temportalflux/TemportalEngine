#pragma once

#include "ecs/types.h"

NS_ECS
struct Component;

class Entity
{

public:
	Identifier id;

	~Entity();

	template <typename TComponent>
	Entity& addComponent(std::shared_ptr<TComponent> pComp)
	{
		return this->addComponent(TComponent::TypeId, pComp);
	}

	template <typename TComponent>
	std::shared_ptr<TComponent> getComponent()
	{
		return std::reinterpret_pointer_cast<TComponent>(
			this->getComponent(TComponent::TypeId)
		);
	}

private:
	struct ComponentEntry
	{
		ComponentTypeId typeId;
		std::shared_ptr<Component> component;
	};
	
	std::array<ComponentEntry, ECS_ENTITY_MAX_COMPONENT_COUNT> mComponents;
	uSize mComponentCount;

	void forEachComponent(std::function<bool(ComponentEntry const& entry)> forEach) const;
	void forEachComponent(std::function<bool(ComponentEntry& entry)> forEach);

	Entity& addComponent(ComponentTypeId const& typeId, std::shared_ptr<Component> pComp);
	std::shared_ptr<Component> getComponent(ComponentTypeId const& typeId);

};

NS_END
