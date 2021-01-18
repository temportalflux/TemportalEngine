#include "ecs/view/ECView.hpp"

#include "ecs/Core.hpp"
#include "network/packet/NetworkPacketECSReplicate.hpp"

using namespace ecs;
using namespace ecs::view;

ecs::EType View::objectType() const { return EType::eView; }

View::View(std::vector<ComponentTypeId> slotTypes)
{
	// Slot types cannot exceed slot capacity. If they do, `ECS_MAX_COMPONENT_VIEW_SLOTS` needs to change.
	assert(slotTypes.size() <= View::SlotCapacity);
	for (uIndex iSlot = 0; iSlot < math::min(View::SlotCapacity, slotTypes.size()); ++iSlot)
	{
		this->mSlots.push(ComponentSlot { slotTypes[iSlot], 0, false });
	}
}

View::~View()
{
	this->mSlots.clear();
}

bool View::hasAllComponents() const
{
	for (auto const& slot : this->mSlots)
	{
		if (!slot.bHasValue) return false;
	}
	return true;
}

void View::onComponentAdded(ComponentTypeId const& typeId, Identifier const& id)
{
	auto idxSlot = this->mSlots.search([typeId](ComponentSlot const& slot) -> ui8
	{
		// typeId <=> slot.typeId
		return typeId < slot.typeId ? -1 : (typeId > slot.typeId ? 1 : 0);
	});
	if (!idxSlot) return;

	auto& slot = this->mSlots[*idxSlot];
	slot.objectId = id;
	slot.bHasValue = true;

	if (ecs::Core::Get()->shouldReplicate())
	{
		ecs::Core::Get()->replicateUpdate(
			ecs::EType::eView, this->typeId(), this->netId()
		)->pushLink(
			ecs::EType::eComponent, slot.typeId,
			ecs::Core::Get()->components().get(slot.typeId, slot.objectId)->netId()
		);
	}

}

component::Component* View::get(ComponentTypeId const& typeId)
{
	for (auto& slot : this->mSlots)
	{
		if (slot.typeId == typeId && slot.bHasValue)
		{
			return ecs::Core::Get()->components().get(slot.typeId, slot.objectId);	
		}
	}
	return nullptr;
}
