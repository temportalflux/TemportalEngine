#include "ecs/system/SystemMovePlayerByInput.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"
#include "ecs/view/ViewPlayerInputMovement.hpp"
#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentPlayerInput.hpp"
#include "ecs/component/ComponentPhysicsController.hpp"
#include "physics/PhysicsScene.hpp"
#include "physics/PhysicsController.hpp"

using namespace ecs;
using namespace ecs::system;

MovePlayerByInput::MovePlayerByInput() : System(view::PlayerInputMovement::TypeId)
{
}

void MovePlayerByInput::update(f32 deltaTime, view::View* view)
{
	OPTICK_EVENT();
	static logging::Logger ControllerLog = DeclareLog("Controller", LOG_INFO);
	
	auto transform = view->get<component::CoordinateTransform>();
	auto input = view->get<component::PlayerInput>();
	auto physController = view->get<component::PhysicsController>();
	assert(transform && input && physController);

	auto orientation = transform->orientation();

	math::Vector3 displacement = math::Vector3::ZERO;
	for (auto mapping : input->axialMoveMappings())
	{
		if (!mapping.bIsActive) continue;
		if (!mapping.bIsGlobal)
		{
			mapping.direction = orientation.rotate(mapping.direction);
			mapping.direction.y() = 0;
			mapping.direction.normalize();
		}
		displacement += mapping.direction * input->axialMoveSpeed() * deltaTime;
	}

	auto& controller = physController->controller();
	if (physController->isAffectedByGravity())
	{
		displacement += controller.scene()->gravity() * deltaTime;
	}
	controller.move(displacement, deltaTime);
	transform->position() = world::Coordinate::fromGlobal(controller.footPosition());

	for (auto& inputAxis : input->lookAxes())
	{
		if (std::abs(inputAxis->delta) > std::numeric_limits<f32>::epsilon())
		{
			// TODO: This isn't really frame independent (doesn't use deltaTime)
			auto rotation = math::Quaternion::FromAxisAngle(
				inputAxis->axis, inputAxis->radians * inputAxis->delta
			);
			
			/* NOTE: on rotational drift
				To account for multi-axis rotations, the axes are applied as different orders of concatenation
				See: https://gamedev.stackexchange.com/a/136175
			*/
			if (inputAxis->bApplyBefore)
			{
				orientation = math::Quaternion::concat(orientation, rotation);
			}
			else
			{
				orientation = math::Quaternion::concat(rotation, orientation);
			}
			inputAxis->delta = 0.0f;
		}
	}

	transform->setOrientation(orientation);
}
