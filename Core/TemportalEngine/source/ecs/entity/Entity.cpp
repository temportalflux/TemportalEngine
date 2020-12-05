#include "ecs/entity/Entity.hpp"

#include "ecs/component/Component.hpp"
#include "ecs/view/ECView.hpp"

using namespace ecs;

Entity::~Entity()
{
	this->mViews.clear();
	this->mComponents.clear();
}

Entity& Entity::addComponent(ComponentTypeId const& typeId, std::shared_ptr<component::Component> pComp)
{
	this->mComponents.insert(ComponentEntry{ typeId, pComp });
	for (auto const& entry : this->mViews)
	{
		entry.ptr->onComponentAdded(typeId, pComp);
	}
	return *this;
}

std::shared_ptr<component::Component> Entity::getComponent(ComponentTypeId const& typeId)
{
	auto idx = this->mComponents.search([typeId](ComponentEntry const& entry)
	{
		// typeId <=> entry.typeId
		return typeId < entry.typeId ? -1 : (typeId > entry.typeId ? 1 : 0);
	});
	return idx ? this->mComponents[*idx].ptr : nullptr;
}

Entity& Entity::addView(ViewTypeId const& typeId, std::shared_ptr<view::View> pView)
{
	this->mViews.insert(ViewEntry{ typeId, pView });
	for (auto const& entry : this->mComponents)
	{
		pView->onComponentAdded(entry.typeId, entry.ptr);
	}
	return *this;
}

std::shared_ptr<view::View> Entity::getView(ViewTypeId const& typeId)
{
	auto idx = this->mViews.search([typeId](ViewEntry const& entry)
	{
		// typeId <=> entry.typeId
		return typeId < entry.typeId ? -1 : (typeId > entry.typeId ? 1 : 0);
	});
	return idx ? this->mViews[*idx].ptr : nullptr;
}

