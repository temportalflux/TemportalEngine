#include "ecs/view/ViewPlayerPhysics.hpp"

#include "Engine.hpp"
#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentPlayerPhysics.hpp"

#include "game/GameInstance.hpp"
#include "world/World.hpp"

using namespace ecs;
using namespace ecs::view;

DEFINE_ECS_VIEW_STATICS(view::PlayerPhysics);
view::PlayerPhysics::PlayerPhysics() : View({
	ecs::component::CoordinateTransform::TypeId,
	ecs::component::PlayerPhysics::TypeId
})
{
}

void view::PlayerPhysics::onComponentReplicationUpdate(ecs::component::Component* component)
{
	if (component->isA<ecs::component::CoordinateTransform>())
	{
		auto ownerNetId = this->owner();
		if (!ownerNetId) return;
		
		auto pWorld = game::Game::Get()->world();
		assert(pWorld);

		if (!pWorld->hasPhysicsController(*ownerNetId)) return;
		auto& controller = pWorld->getPhysicsController(*ownerNetId);
		
		auto* pTransform = dynamic_cast<ecs::component::CoordinateTransform*>(component);
		auto const transformPos = pTransform->position().toGlobal();
		auto posDiff = controller.footPosition() - transformPos;
		if (posDiff.magnitudeSq() > 0.001f)
		{
			DeclareLog("Update", LOG_DEBUG).log(
				LOG_DEBUG, "Correcting player from %s -> %s (diffmag %0.5f)",
				controller.footPosition().toString().c_str(),
				transformPos.toString().c_str(),
				posDiff.magnitudeSq()
			);
			controller.setFootPosition(transformPos);
		}
	}
}
