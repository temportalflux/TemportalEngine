#include "ecs/system/SystemMovePlayerByInput.hpp"

#include "Engine.hpp"
#include "ecs/view/ViewPlayerInputMovement.hpp"
#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentPlayerPhysics.hpp"
#include "game/GameInstance.hpp"
#include "game/GameClient.hpp"
#include "input/InputCore.hpp"
#include "input/Queue.hpp"
#include "logging/Logger.hpp"
#include "physics/PhysicsScene.hpp"
#include "physics/PhysicsController.hpp"
#include "world/World.hpp"

using namespace evcs;
using namespace evcs::system;

MovePlayerByInput::MovePlayerByInput() : System(view::PlayerInputMovement::TypeId)
{
	this->mAxialMoveSpeed = 4.0f;
	this->mAxisMappings = { {
		// Forward
		{ input::EKey::W, math::V3_FORWARD, false },
		// Backward
		{ input::EKey::S , -math::V3_FORWARD, false },
		// Strafe Right
		{ input::EKey::D, math::V3_RIGHT, false },
		// Strafe Left
		{ input::EKey::A , -math::V3_RIGHT, false },
		// Up
		{ input::EKey::E, math::V3_UP, true },
		// Down
		{ input::EKey::Q, -math::V3_UP, true }
	} };
	this->mLookHorizontal = { math::V3_UP, -math::toRadians(90.0f), false };
	this->mLookVertical = { math::V3_RIGHT, -math::toRadians(90.0f), true };
}

void MovePlayerByInput::subscribeToQueue()
{
	// Its not /great/ form to have events modifying the component directly, but its honestly the easiest
	auto inputQueue = engine::Engine::Get()->getInputQueue();
#define REGISTER_INPUT(EVENT, FUNC_PTR) inputQueue->OnInputEvent.bind(EVENT, this->weak_from_this(), std::bind(FUNC_PTR, this, std::placeholders::_1))
	REGISTER_INPUT(input::EInputType::KEY, &MovePlayerByInput::onKeyInput);
	REGISTER_INPUT(input::EInputType::MOUSE_MOVE, &MovePlayerByInput::onMouseMove);
#undef REGISTER_INPUT
}

void MovePlayerByInput::onKeyInput(input::Event const & evt)
{
	if (input::isTextInputActive()) return;
	for (auto& mapping : this->mAxisMappings)
	{
		if (mapping.key == evt.inputKey.key)
		{
			if (evt.inputKey.action == input::EAction::PRESS)
				mapping.bIsActive = true;
			if (evt.inputKey.action == input::EAction::RELEASE)
				mapping.bIsActive = false;
		}
	}
}

void MovePlayerByInput::onMouseMove(input::Event const & evt)
{
	this->mLookHorizontal.delta = evt.inputMouseMove.xDelta;
	this->mLookVertical.delta = evt.inputMouseMove.yDelta;
}

void MovePlayerByInput::update(f32 deltaTime, view::View* view)
{
	OPTICK_EVENT();
	static logging::Logger ControllerLog = DeclareLog("Controller", LOG_INFO);

	auto transform = view->get<component::CoordinateTransform>();
	auto physicsComp = view->get<component::PlayerPhysics>();
	assert(transform && physicsComp);
	assert(physicsComp->owner()); // assumes that transform and physics have the same owner

	bool bHasMoved = false;
	auto displacement = math::Vector3::ZERO;
	for (auto mapping : this->mAxisMappings)
	{
		if (!mapping.bIsActive) continue;
		if (!mapping.bIsGlobal)
		{
			mapping.direction = transform->orientation().rotate(mapping.direction);
			mapping.direction.y() = 0;
			mapping.direction.normalize();
		}
		displacement += mapping.direction * this->mAxialMoveSpeed * deltaTime;
		bHasMoved = true;
	}
	if (bHasMoved)
	{
		auto pWorld = game::Game::Get()->world();
		auto ownerNetId = physicsComp->owner().value();
		auto& controller = pWorld->getPhysicsController(ownerNetId);
		controller.move(displacement, deltaTime);
		transform->setPosition(world::Coordinate::fromGlobal(controller.footPosition()));
	}

	bool bChangedOrientation = false;
	auto orientation = transform->orientation();
	auto lookAxes = std::vector<InputAxis*>({ &this->mLookVertical, &this->mLookHorizontal });
	for (auto* inputAxis : lookAxes)
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
			bChangedOrientation = true;
		}
	}
	if (bChangedOrientation)
	{
		auto& ecs = engine::Engine::Get()->getECS();
		ecs.beginReplication();
		transform->setOrientation(orientation);
		ecs.endReplication();
	}

}
