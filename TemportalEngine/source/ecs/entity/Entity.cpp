#include "ecs/entity/Entity.hpp"

#include "ecs/component/Component.hpp"

using namespace ecs;

Entity::~Entity()
{
	// All components are automatically deleted because the shared_ptrs go out of scope
}

void Entity::forEachComponent(std::function<bool(ComponentEntry const& entry)> forEach) const
{
	for (uIndex i = 0; i < this->mComponentCount; ++i)
	{
		if (forEach(this->mComponents[i])) break;
	}
}

void Entity::forEachComponent(std::function<bool(ComponentEntry& entry)> forEach)
{
	for (uIndex i = 0; i < this->mComponentCount; ++i)
	{
		if (forEach(this->mComponents[i])) break;
	}
}

Entity& Entity::addComponent(ComponentTypeId const& typeId, std::shared_ptr<Component> pComp)
{
	static uSize COMP_ENTRY_SIZE = sizeof(ComponentEntry);
	assert(this->mComponentCount < this->mComponents.size());
	// TODO: Could this be a binary/upper-bound search?
	uIndex desiredIdx = 0;
	while (
		desiredIdx < this->mComponentCount
		&& this->mComponents[desiredIdx].typeId < typeId
	)
	{
		desiredIdx++;
	}
	if (desiredIdx < this->mComponentCount)
	{
		uSize sizeShift = (this->mComponentCount - desiredIdx) * COMP_ENTRY_SIZE;
		memcpy_s(
			// Shift all elements at desired and after over by 1
			this->mComponents.data() + (desiredIdx + 1) * COMP_ENTRY_SIZE, sizeShift,
			this->mComponents.data() + (desiredIdx) * COMP_ENTRY_SIZE, sizeShift
		);
	}
	this->mComponents[desiredIdx] = { typeId, pComp };
	this->mComponentCount++;
	return *this;
}

std::shared_ptr<Component> Entity::getComponent(ComponentTypeId const& typeId)
{
	std::shared_ptr<Component> comp = nullptr;
	this->forEachComponent([typeId, &comp](auto const& entry) -> bool
	{
		if (entry.typeId == typeId)
		{
			comp = entry.component;
			return true;
		}
		return false;
	});
	return comp;
}
