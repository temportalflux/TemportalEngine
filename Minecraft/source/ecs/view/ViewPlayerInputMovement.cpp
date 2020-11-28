#include "ecs/view/ViewPlayerInputMovement.hpp"

#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentPlayerInput.hpp"

using namespace ecs;
using namespace ecs::view;

DEFINE_ECS_VIEW_STATICS(PlayerInputMovement);
PlayerInputMovement::PlayerInputMovement() : View({
	ecs::CoordinateTransform::TypeId, ecs::PlayerInput::TypeId
}) {}
