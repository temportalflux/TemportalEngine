#include "ecs/component/Component.hpp"

#include "ecs/Core.hpp"
#include "network/packet/NetworkPacketECSReplicate.hpp"

using namespace ecs::component;

ecs::EType Component::objectType() const { return EType::eComponent; }

std::shared_ptr<network::packet::ECSReplicate> Component::replicateUpdate()
{
	return ecs::Core::Get()->replicateUpdate(
		ecs::EType::eComponent, this->typeId(), this->netId()
	);
}

std::vector<Component::Field> Component::allFields() const { return {}; }

void Component::onReplicateUpdate()
{
	auto* ecs = ecs::Core::Get();
	auto ownerEntityId = ecs->components().getComponentEntityId(this->typeId(), this->id());
	if (ownerEntityId)
	{
		auto pEntity = ecs->entities().get(*ownerEntityId);
		for (auto* pView : pEntity->views())
		{
			if (pView->includesComponent(this->typeId(), this->id()))
			{
				pView->onComponentReplicationUpdate(this);
			}
		}
	}
}
