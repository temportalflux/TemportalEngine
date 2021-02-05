#include "ecs/entity/Entity.hpp"

#include "Engine.hpp"
#include "ecs/Core.hpp"
#include "ecs/component/Component.hpp"
#include "ecs/view/ECView.hpp"
#include "network/packet/NetworkPacketECSReplicate.hpp"

using namespace evcs;

view::View* getViewAt(TypeId typeId, Identifier id)
{
	return evcs::Core::Get()->views().get(id);
}

component::Component* getCompAt(TypeId typeId, Identifier id)
{
	return evcs::Core::Get()->components().get(typeId, id);
}

Entity::~Entity()
{
	auto* ecs = evcs::Core::Get();
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

EType Entity::objectType() const { return EType::eEntity; }
TypeId Entity::typeId() const { return 0; }

void Entity::setOwner(std::optional<ui32> ownerNetId)
{
	for (auto const& slot : this->mComponents)
	{
		getCompAt(slot.typeId, slot.objectId)->setOwner(ownerNetId);
	}
	for (auto const& slot : this->mViews)
	{
		getViewAt(slot.typeId, slot.objectId)->setOwner(ownerNetId);
	}
	IEVCSObject::setOwner(ownerNetId);
}

void Entity::kill()
{
	evcs::Core::Get()->entities().release(this->id());
}

Entity& Entity::addComponent(ComponentTypeId const& typeId, component::Component* pComp)
{
	pComp->setOwner(this->owner());

	this->mComponents.insert(ItemEntry { typeId, pComp->id() });
	for (auto const& slot : this->mViews)
	{
		getViewAt(slot.typeId, slot.objectId)->onComponentAdded(typeId, pComp->id());
	}

	evcs::Core::Get()->components().setComponentEntity(typeId, pComp->id(), this->id());

	if (evcs::Core::Get()->shouldReplicate())
	{
		evcs::Core::Get()->replicateUpdate(
			evcs::EType::eEntity, 0, this->netId()
		)->pushLink(
			evcs::EType::eComponent, typeId, pComp->netId()
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
	return evcs::Core::Get()->components().get(typeId, slot.objectId);
}

Entity& Entity::addView(ViewTypeId const& typeId, view::View* pView)
{
	pView->setOwner(this->owner());

	this->mViews.insert(ItemEntry { typeId, pView->id() });
	for (auto const& entry : this->mComponents)
	{
		pView->onComponentAdded(entry.typeId, entry.objectId);
	}

	if (evcs::Core::Get()->shouldReplicate())
	{
		evcs::Core::Get()->replicateUpdate(
			evcs::EType::eEntity, 0, this->netId()
		)->pushLink(
			evcs::EType::eView, typeId, pView->netId()
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
	return evcs::Core::Get()->views().get(slot.objectId);
}

Entity::ViewIterator::ViewIterator(Entity* entity)
	: mpEntity(entity)
{
}

Entity::ViewIteratorEntry Entity::ViewIterator::begin()
{
	return ViewIteratorEntry(this->mpEntity, this->mpEntity->mViews.begin());
}

Entity::ViewIteratorEntry Entity::ViewIterator::end()
{
	return ViewIteratorEntry(this->mpEntity, this->mpEntity->mViews.end());
}

Entity::ViewIteratorEntry::ViewIteratorEntry(Entity* entity, ViewArray::iterator iter)
	: mpEntity(entity), mEntityIter(iter)
{
}

Entity::ViewIteratorEntry::ViewIteratorEntry(ViewIteratorEntry const& other)
	: mpEntity(other.mpEntity), mEntityIter(other.mEntityIter)
{
}

view::View* Entity::ViewIteratorEntry::operator*()
{
	auto& slot = *(this->mEntityIter);
	return evcs::Core::Get()->views().get(slot.objectId);
}

void Entity::ViewIteratorEntry::operator++()
{
	++this->mEntityIter;
}

bool Entity::ViewIteratorEntry::operator!=(ViewIteratorEntry const& other)
{
	return this->mEntityIter != other.mEntityIter;
}

Entity::ViewIterator Entity::views()
{
	return ViewIterator(this);
}
