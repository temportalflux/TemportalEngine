#include "evcs/view/ViewPlayerPhysics.hpp"

#include "Engine.hpp"
#include "evcs/component/CoordinateTransform.hpp"
#include "evcs/component/ComponentPlayerPhysics.hpp"

#include "game/GameInstance.hpp"
#include "world/World.hpp"

using namespace evcs;
using namespace evcs::component;
using namespace evcs::view;

DEFINE_ECS_VIEW_STATICS(view::PlayerPhysics);
view::PlayerPhysics::PlayerPhysics() : View({
	CoordinateTransform::TypeId,
	PlayerPhysics::TypeId
})
{
}

void view::PlayerPhysics::onComponentReplicationUpdate(Component* component)
{
	if (component->isA<CoordinateTransform>())
	{
		auto ownerNetId = this->owner();
		if (!ownerNetId) return;
		
		auto pWorld = game::Game::Get()->world();
		assert(pWorld);

		if (!pWorld->hasPhysicsController(*ownerNetId)) return;
		auto& controller = pWorld->getPhysicsController(*ownerNetId);
		
		auto* pTransform = dynamic_cast<CoordinateTransform*>(component);
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
