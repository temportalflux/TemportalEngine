#include "ecs/view/ECView.hpp"

using namespace ecs;
using namespace ecs::view;

View::View(std::vector<ComponentTypeId> slotTypes)
{
	// Slot types cannot exceed slot capacity. If they do, `ECS_MAX_COMPONENT_VIEW_SLOTS` needs to change.
	assert(slotTypes.size() <= View::SlotCapacity);
	for (uIndex iSlot = 0; iSlot < math::min(View::SlotCapacity, slotTypes.size()); ++iSlot)
	{
		this->mSlots.push(ComponentSlot { slotTypes[iSlot], std::weak_ptr<component::Component>() });
	}
}

View::~View()
{
	this->mSlots.clear();
}

Identifier const& View::id() const { return this->mId; }

bool View::hasAllComponents() const
{
	for (auto const& slot : this->mSlots)
	{
		if (slot.component.expired()) return false;
	}
	return true;
}

void View::onComponentAdded(ComponentTypeId const& typeId, std::weak_ptr<component::Component> const& ptr)
{
	auto idxSlot = this->mSlots.search([typeId](ComponentSlot const& slot) -> ui8
	{
		// typeId <=> slot.typeId
		return typeId < slot.typeId ? -1 : (typeId > slot.typeId ? 1 : 0);
	});
	if (!idxSlot) return;
	this->mSlots[*idxSlot].component = ptr;
}

std::shared_ptr<component::Component> View::lockComponent(ComponentTypeId const& typeId)
{
	for (auto& slot : this->mSlots)
	{
		if (slot.typeId == typeId)
		{
			std::shared_ptr<component::Component> component = nullptr;
			if (!slot.component.expired())
			{
				component = slot.component.lock();
			}
			return component;
		}
	}
	return nullptr;
}
