#pragma once

#include "TemportalEnginePCH.hpp"

#include "FixedSortedArray.hpp"
#include "ecs/types.h"

NS_ECS
FORWARD_DEF(NS_COMPONENT, class Component);
NS_END

#define DECLARE_ECS_VIEW_STATICS() public: static ViewTypeId TypeId;
#define DEFINE_ECS_VIEW_STATICS(COMP_TYPE) ViewTypeId COMP_TYPE::TypeId = 0;

NS_ECS
NS_VIEW
class Manager;

class View
{
	friend class Manager;
	static inline constexpr uSize SlotCapacity = ECS_MAX_COMPONENT_VIEW_SLOTS;

public:
	View() = default;
	View(std::vector<ComponentTypeId> slotTypes);
	Identifier const& id() const;

	bool hasAllComponents() const;

	void onComponentAdded(ComponentTypeId const& typeId, std::weak_ptr<component::Component> const& ptr);

	template <typename TComponent>
	std::shared_ptr<TComponent> get()
	{
		return std::reinterpret_pointer_cast<TComponent>(
			lockComponent(TComponent::TypeId)
		);
	}

private:
	struct ComponentSlot
	{
		ComponentTypeId typeId;
		std::weak_ptr<component::Component> component;
		bool operator<(ComponentSlot const& other) const { return typeId < other.typeId; }
		bool operator>(ComponentSlot const& other) const { return typeId > other.typeId; }
	};

	Identifier mId;
	FixedSortedArray<ComponentSlot, SlotCapacity> mSlots;

	std::shared_ptr<component::Component> lockComponent(ComponentTypeId const& typeId);

};

NS_END
NS_END
