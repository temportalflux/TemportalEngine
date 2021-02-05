#include "evcs/component/Component.hpp"

#include "evcs/Core.hpp"
#include "network/packet/NetworkPacketECSReplicate.hpp"

using namespace evcs;
using namespace evcs::component;

evcs::EType Component::objectType() const { return EType::eComponent; }

std::shared_ptr<network::packet::EVCSReplicate> Component::replicateUpdate()
{
	return Core::Get()->replicateUpdate(
		EType::eComponent, this->typeId(), this->netId()
	);
}

std::vector<Component::Field> Component::allFields() const { return {}; }

void Component::onReplicateUpdate()
{
	auto* ecs = evcs::Core::Get();
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
