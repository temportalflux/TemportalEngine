#include "ecs/entity/Entity.hpp"

#include "Engine.hpp"
#include "ecs/Core.hpp"
#include "ecs/component/Component.hpp"
#include "ecs/view/ECView.hpp"
#include "network/packet/NetworkPacketECSReplicate.hpp"

using namespace ecs;

Entity::~Entity()
{
	auto* ecs = ecs::Core::Get();
	for (auto const& slot : this->mViews)
	{
		ecs->views().destroy(slot.typeId, slot.objectId);
	}
	this->mViews.clear();
	for (auto const& slot : this->mComponents)
	{
		ecs->components().destroy(slot.typeId, slot.objectId);
	}
	this->mComponents.clear();
}

ecs::EType Entity::objectType() const { return EType::eEntity; }
ecs::TypeId Entity::typeId() const { return 0; }

void Entity::kill()
{
	ecs::Core::Get()->entities().release(this->id);
}

Entity& Entity::addComponent(ComponentTypeId const& typeId, component::Component* pComp)
{
	this->mComponents.insert(ItemEntry { typeId, pComp->id });
	for (auto const& entry : this->mViews)
	{
		ecs::Core::Get()->views().get(entry.objectId)->onComponentAdded(typeId, pComp->id);
	}

	if (ecs::Core::Get()->shouldReplicate())
	{
		ecs::Core::Get()->replicateUpdate(
			ecs::EType::eEntity, 0, this->netId
		)->pushLink(
			ecs::EType::eComponent, typeId, pComp->netId
		);
	}

	return *this;
}

component::Component* Entity::getComponent(ComponentTypeId const& typeId)
{
	auto idx = this->mComponents.search([typeId](ItemEntry const& entry)
	{
		// typeId <=> entry.typeId
		return typeId < entry.typeId ? -1 : (typeId > entry.typeId ? 1 : 0);
	});
	return idx ? ecs::Core::Get()->components().get(typeId, this->mComponents[*idx].objectId) : nullptr;
}

Entity& Entity::addView(ViewTypeId const& typeId, view::View* pView)
{
	this->mViews.insert(ItemEntry { typeId, pView->id });
	for (auto const& entry : this->mComponents)
	{
		pView->onComponentAdded(entry.typeId, entry.objectId);
	}

	if (ecs::Core::Get()->shouldReplicate())
	{
		ecs::Core::Get()->replicateUpdate(
			ecs::EType::eEntity, 0, this->netId
		)->pushLink(
			ecs::EType::eView, typeId, pView->netId
		);
	}

	return *this;
}

view::View* Entity::getView(ViewTypeId const& typeId)
{
	auto idx = this->mViews.search([typeId](ItemEntry const& entry)
	{
		// typeId <=> entry.typeId
		return typeId < entry.typeId ? -1 : (typeId > entry.typeId ? 1 : 0);
	});
	return idx ? ecs::Core::Get()->views().get(this->mViews[*idx].objectId) : nullptr;
}

