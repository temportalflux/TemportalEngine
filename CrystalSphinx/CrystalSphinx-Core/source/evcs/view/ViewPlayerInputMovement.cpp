#include "evcs/view/ViewPlayerInputMovement.hpp"

#include "evcs/component/CoordinateTransform.hpp"
#include "evcs/component/ComponentPlayerPhysics.hpp"

using namespace evcs;
using namespace evcs::component;
using namespace evcs::view;

DEFINE_ECS_VIEW_STATICS(PlayerInputMovement);
PlayerInputMovement::PlayerInputMovement() : View({
	CoordinateTransform::TypeId,
	PlayerPhysics::TypeId
}) {}
