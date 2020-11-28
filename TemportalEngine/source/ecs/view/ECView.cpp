#include "ecs/view/ECView.hpp"

using namespace ecs;
using namespace ecs::view;

View::View(std::vector<ComponentTypeId> slotTypes)
{
	for (uIndex iSlot = 0; iSlot < math::min(View::SlotCapacity, slotTypes.size()); ++iSlot)
	{
		this->mSlots.push(ComponentSlot { slotTypes[iSlot] });
	}
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

void View::onComponentAdded(ComponentTypeId const& typeId, std::weak_ptr<Component> const& ptr)
{
	auto idxSlot = this->mSlots.search([typeId](ComponentSlot const& slot) -> ui8
	{
		// slot.typeId <=> typeId
		return slot.typeId < typeId ? -1 : (slot.typeId > typeId ? 1 : 0);
	});
	if (!idxSlot) return;
	this->mSlots[*idxSlot].component = ptr;
}

std::shared_ptr<Component> View::lockComponent(ComponentTypeId const& typeId)
{
	for (auto& slot : this->mSlots)
	{
		if (slot.typeId == typeId)
		{
			std::shared_ptr<Component> component = nullptr;
			if (!slot.component.expired())
			{
				component = slot.component.lock();
			}
			return component;
		}
	}
	return nullptr;
}
