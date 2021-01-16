#pragma once

#include "ecs/types.h"

#include "dataStructures/FixedArray.hpp"

NS_ECS
FORWARD_DEF(NS_COMPONENT, class Component);
FORWARD_DEF(NS_VIEW, class View);

class Entity
{

	template <typename TItemId, typename TItem>
	struct ItemEntry
	{
		TItemId typeId;
		std::shared_ptr<TItem> ptr;
		bool operator<(ItemEntry<TItemId, TItem> const& other) const { return typeId < other.typeId; }
		bool operator>(ItemEntry<TItemId, TItem> const& other) const { return typeId > other.typeId; }
	};
	
	using ComponentEntry = ItemEntry<ComponentTypeId, component::Component>;
	using ViewEntry = ItemEntry<ViewTypeId, view::View>;

public:
	Identifier id;

	~Entity();

#pragma region Components

public:

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

	FixedArray<ComponentEntry, ECS_ENTITY_MAX_COMPONENT_COUNT> mComponents;

	Entity& addComponent(ComponentTypeId const& typeId, std::shared_ptr<component::Component> pComp);
	std::shared_ptr<component::Component> getComponent(ComponentTypeId const& typeId);

#pragma endregion

#pragma region Views

public:

	template <typename TView>
	Entity& addView(std::shared_ptr<TView> pView)
	{
		return this->addView(TView::TypeId, pView);
	}

	template <typename TView>
	std::shared_ptr<TView> getView()
	{
		return std::reinterpret_pointer_cast<TView>(
			this->getView(TView::TypeId)
		);
	}

private:
	FixedArray<ViewEntry, ECS_MAX_VIEWS_PER_ENTITY_COUNT> mViews;

	Entity& addView(ViewTypeId const& typeId, std::shared_ptr<view::View> pView);
	std::shared_ptr<view::View> getView(ViewTypeId const& typeId);

#pragma endregion



};

NS_END
