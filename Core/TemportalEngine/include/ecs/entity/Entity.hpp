#pragma once

#include "ecs/IEVCSObject.hpp"

#include "dataStructures/FixedArray.hpp"

NS_EVCS
FORWARD_DEF(NS_COMPONENT, class Component);
FORWARD_DEF(NS_VIEW, class View);

class Entity : public evcs::IEVCSObject
{

	struct ItemEntry
	{
		TypeId typeId;
		Identifier objectId;
		bool operator<(ItemEntry const& other) const { return typeId < other.typeId; }
		bool operator>(ItemEntry const& other) const { return typeId > other.typeId; }
	};
	
public:

	~Entity();
	EType objectType() const;
	TypeId typeId() const;
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
	using ViewArray = FixedArray<ItemEntry, ECS_MAX_VIEWS_PER_ENTITY_COUNT>;

	class ViewIteratorEntry
	{
	public:
		ViewIteratorEntry(Entity* entity, ViewArray::iterator iter);
		ViewIteratorEntry(ViewIteratorEntry const& other);

		view::View* operator*();
		void operator++();
		bool operator!=(ViewIteratorEntry const& other);

	private:
		Entity* mpEntity;
		ViewArray::iterator mEntityIter;
	};
	class ViewIterator
	{
	public:
		ViewIterator(Entity* entity);
		ViewIteratorEntry begin();
		ViewIteratorEntry end();
	private:
		Entity* mpEntity;
	};

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

	ViewIterator views();

private:
	ViewArray mViews;

	view::View* getView(ViewTypeId const& typeId);

#pragma endregion

};

NS_END
