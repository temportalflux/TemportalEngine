#pragma once

#include "ecs/IEVCSObject.hpp"

#include "dataStructures/FixedArray.hpp"

NS_ECS
FORWARD_DEF(NS_COMPONENT, class Component);
NS_END

#define DECLARE_ECS_VIEW_STATICS() \
	public: \
		static ViewTypeId TypeId; \
		static void construct(ecs::view::View* ptr); \
		ecs::TypeId typeId() const override;
#define DEFINE_ECS_VIEW_STATICS(COMP_TYPE) \
	ViewTypeId COMP_TYPE::TypeId = 0; \
	void COMP_TYPE::construct(ecs::view::View* ptr) { new (ptr) COMP_TYPE(); } \
	ecs::TypeId COMP_TYPE::typeId() const { return COMP_TYPE::TypeId; }

NS_ECS
NS_VIEW
class Manager;

class View : public ecs::IEVCSObject
{
	static inline constexpr uSize SlotCapacity = ECS_MAX_COMPONENT_VIEW_SLOTS;

public:
	View() = default;
	View(std::vector<ComponentTypeId> slotTypes);
	~View();

	ecs::EType objectType() const;
	bool hasAllComponents() const;
	void onComponentAdded(ComponentTypeId const& typeId, Identifier const& id);

	template <typename TComponent>
	TComponent* get()
	{
		return reinterpret_cast<TComponent*>(this->get(TComponent::TypeId));
	}

private:
	struct ComponentSlot
	{
		ecs::TypeId typeId;
		Identifier objectId;
		bool bHasValue;
		bool operator<(ComponentSlot const& other) const { return typeId < other.typeId; }
		bool operator>(ComponentSlot const& other) const { return typeId > other.typeId; }
	};

	FixedArray<ComponentSlot, SlotCapacity> mSlots;

	component::Component* get(ComponentTypeId const& typeId);

};

NS_END
NS_END
