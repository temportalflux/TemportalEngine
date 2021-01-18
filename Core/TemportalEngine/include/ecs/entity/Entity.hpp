#pragma once

#include "ecs/IEVCSObject.hpp"

#include "dataStructures/FixedArray.hpp"

NS_ECS
FORWARD_DEF(NS_COMPONENT, class Component);
FORWARD_DEF(NS_VIEW, class View);

class Entity : public ecs::IEVCSObject
{

	struct ItemEntry
	{
		ecs::TypeId typeId;
		Identifier objectId;
		bool operator<(ItemEntry const& other) const { return typeId < other.typeId; }
		bool operator>(ItemEntry const& other) const { return typeId > other.typeId; }
	};
	
public:

	~Entity();
	ecs::EType objectType() const;
	ecs::TypeId typeId() const;
	void setOwner(std::optional<ui32> ownerNetId) override;

	/**
	 * Causes the entity to not be held onto by the entity manager.
	 * When the shared_ptr of the entity has no more references, it will truely be removed/deleted.
	 * If this is the only reference to the entity, this will happen immediately.
	 */
	void kill();

#pragma region Components

public:
	Entity& addComponent(ComponentTypeId const& typeId, component::Component* pComp);

	template <typename TComponent>
	Entity& addComponent(TComponent* pComp)
	{
		return this->addComponent(TComponent::TypeId, pComp);
	}

	template <typename TComponent>
	TComponent* getComponent()
	{
		return reinterpret_cast<TComponent*>(
			this->getComponent(TComponent::TypeId)
		);
	}

private:

	FixedArray<ItemEntry, ECS_ENTITY_MAX_COMPONENT_COUNT> mComponents;

	component::Component* getComponent(ComponentTypeId const& typeId);

#pragma endregion

#pragma region Views

public:
	Entity& addView(ViewTypeId const& typeId, view::View* pView);

	template <typename TView>
	Entity& addView(TView* pView)
	{
		return this->addView(TView::TypeId, pView);
	}

	template <typename TView>
	TView* getView()
	{
		return reinterpret_cast<TView*>(
			this->getView(TView::TypeId)
		);
	}

private:
	FixedArray<ItemEntry, ECS_MAX_VIEWS_PER_ENTITY_COUNT> mViews;

	view::View* getView(ViewTypeId const& typeId);

#pragma endregion



};

NS_END
