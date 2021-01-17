#pragma once

#include "ecs/IEVCSObject.hpp"

#include "dataStructures/FixedArray.hpp"

NS_ECS
FORWARD_DEF(NS_COMPONENT, class Component);
NS_END

#define DECLARE_ECS_VIEW_STATICS() public: static ViewTypeId TypeId;
#define DEFINE_ECS_VIEW_STATICS(COMP_TYPE) ViewTypeId COMP_TYPE::TypeId = 0;

NS_ECS
NS_VIEW
class Manager;

class View : public ecs::IEVCSObject
{
	static inline constexpr uSize SlotCapacity = ECS_MAX_COMPONENT_VIEW_SLOTS;

public:
	View() = default;
	~View();

	void setComponentSlots(std::vector<ComponentTypeId> slotTypes);
	
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

	FixedArray<ComponentSlot, SlotCapacity> mSlots;

	std::shared_ptr<component::Component> lockComponent(ComponentTypeId const& typeId);

};

NS_END
NS_END
