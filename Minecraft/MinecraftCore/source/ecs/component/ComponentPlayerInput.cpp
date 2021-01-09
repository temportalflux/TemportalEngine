#include "ecs/component/ComponentPlayerInput.hpp"

#include "Engine.hpp"
#include "input/InputCore.hpp"
#include "input/Queue.hpp"

using namespace ecs;
using namespace ecs::component;

DEFINE_ECS_COMPONENT_STATICS(PlayerInput)

PlayerInput::PlayerInput()
{
	this->mAxialMoveSpeed = 4.0f;
	this->mAxisMappings = {{
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
	}};
	this->mLookHorizontal = { math::V3_UP, -math::toRadians(90.0f), false };
	this->mLookVertical = { math::V3_RIGHT, -math::toRadians(90.0f), true };
}

PlayerInput::~PlayerInput()
{
}

void PlayerInput::subscribeToQueue()
{
	// Its not /great/ form to have events modifying the component directly, but its honestly the easiest
	auto inputQueue = engine::Engine::Get()->getInputQueue();
#define REGISTER_INPUT(EVENT, FUNC_PTR) inputQueue->OnInputEvent.bind(EVENT, this->weak_from_this(), std::bind(FUNC_PTR, this, std::placeholders::_1))
	REGISTER_INPUT(input::EInputType::KEY, &PlayerInput::onKeyInput);
	REGISTER_INPUT(input::EInputType::MOUSE_MOVE, &PlayerInput::onMouseMove);
#undef REGISTER_INPUT
}

void PlayerInput::onKeyInput(input::Event const & evt)
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

void PlayerInput::onMouseMove(input::Event const & evt)
{
	this->mLookHorizontal.delta = evt.inputMouseMove.xDelta;
	this->mLookVertical.delta = evt.inputMouseMove.yDelta;
}

f32 const& PlayerInput::axialMoveSpeed() const
{
	return this->mAxialMoveSpeed;
}

std::array<PlayerInput::InputMapping, 6> const& PlayerInput::axialMoveMappings() const
{
	return this->mAxisMappings;
}

std::vector<PlayerInput::InputAxis*> PlayerInput::lookAxes()
{
	return { &this->mLookVertical, &this->mLookHorizontal };
}
