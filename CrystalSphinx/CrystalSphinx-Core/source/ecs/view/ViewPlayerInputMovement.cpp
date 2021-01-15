#include "ecs/view/ViewPlayerInputMovement.hpp"

#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentPlayerInput.hpp"
#include "ecs/component/ComponentPhysicsController.hpp"

using namespace ecs;
using namespace ecs::view;

DEFINE_ECS_VIEW_STATICS(PlayerInputMovement);
PlayerInputMovement::PlayerInputMovement() : View({
	ecs::component::CoordinateTransform::TypeId,
	ecs::component::PlayerInput::TypeId,
	ecs::component::PhysicsController::TypeId
}) {}
