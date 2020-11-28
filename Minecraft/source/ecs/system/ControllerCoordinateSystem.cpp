#include "ecs/system/ControllerCoordinateSystem.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"
#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/PlayerInputComponent.hpp"

using namespace ecs;

void ControllerCoordinateSystem::update(
	f32 deltaTime,
	std::shared_ptr<ecs::CoordinateTransform> transform,
	std::shared_ptr<ecs::PlayerInputComponent> input
)
{
	OPTICK_EVENT();
	static logging::Logger ControllerLog = DeclareLog("Controller");

	auto orientation = transform->orientation();
	auto euler = orientation.euler();
	auto rot = math::Quaternion::FromAxisAngle(math::Vector3unitY, euler.y());

	for (auto const& mapping : input->axialMoveMappings())
	{
		if (!mapping.bIsActive) continue;
		auto dir = mapping.bIsGlobal ? mapping.direction : rot.rotate(mapping.direction);
		transform->move(dir * deltaTime * input->axialMoveSpeed());
	}

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
