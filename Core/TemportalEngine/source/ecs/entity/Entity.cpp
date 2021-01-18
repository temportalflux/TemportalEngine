#include "ecs/entity/Entity.hpp"

#include "Engine.hpp"
#include "ecs/Core.hpp"
#include "ecs/component/Component.hpp"
#include "ecs/view/ECView.hpp"
#include "network/packet/NetworkPacketECSReplicate.hpp"

using namespace ecs;

view::View* getViewAt(ecs::TypeId typeId, ecs::Identifier id)
{
	return ecs::Core::Get()->views().get(id);
}

component::Component* getCompAt(ecs::TypeId typeId, ecs::Identifier id)
{
	return ecs::Core::Get()->components().get(typeId, id);
}

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

void Entity::setOwner(std::optional<ui32> ownerNetId)
{
	for (auto const& slot : this->mViews)
	{
		getViewAt(slot.typeId, slot.objectId)->setOwner(ownerNetId);
	}
	for (auto const& slot : this->mComponents)
	{
		getCompAt(slot.typeId, slot.objectId)->setOwner(ownerNetId);
	}
	IEVCSObject::setOwner(ownerNetId);
}

void Entity::kill()
{
	ecs::Core::Get()->entities().release(this->id);
}

Entity& Entity::addComponent(ComponentTypeId const& typeId, component::Component* pComp)
{
	pComp->setOwner(this->owner());

	this->mComponents.insert(ItemEntry { typeId, pComp->id });
	for (auto const& slot : this->mViews)
	{
		getViewAt(slot.typeId, slot.objectId)->onComponentAdded(typeId, pComp->id);
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
	if (!idx) return nullptr;
	auto& slot = this->mComponents[*idx];
	return ecs::Core::Get()->components().get(typeId, slot.objectId);
}

Entity& Entity::addView(ViewTypeId const& typeId, view::View* pView)
{
	pView->setOwner(this->owner());

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
	if (!idx) return nullptr;
	auto& slot = this->mViews[*idx];
	return ecs::Core::Get()->views().get(slot.objectId);
}

