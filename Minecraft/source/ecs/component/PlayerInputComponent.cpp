#include "ecs/component/PlayerInputComponent.hpp"

#include "Engine.hpp"
#include "input/Queue.hpp"

using namespace ecs;

ComponentTypeId PlayerInputComponent::TypeId = 0;

PlayerInputComponent::PlayerInputComponent()
{
	this->mAxialMoveSpeed = 4.0f;
	this->mAxisMappings = {{
		// Forward
		{ input::EKey::W, math::Vector3unitZ, false },
		// Backward
		{ input::EKey::S , -math::Vector3unitZ, false },
		// Strafe Right
		{ input::EKey::D, math::Vector3unitX, false },
		// Strafe Left
		{ input::EKey::A , -math::Vector3unitX, false },
		// Up
		{ input::EKey::E, math::Vector3unitY, true },
		// Down
		{ input::EKey::Q, -math::Vector3unitY, true }
	}};
	this->mLookHorizontal = { math::Vector3unitY, math::toRadians(90.0f), false };
	this->mLookVertical = { math::Vector3unitX, math::toRadians(90.0f), true };
}

PlayerInputComponent::~PlayerInputComponent()
{
}

void PlayerInputComponent::subscribeToQueue()
{
	// Its not /great/ form to have events modifying the component directly, but its honestly the easiest
	auto inputQueue = engine::Engine::Get()->getInputQueue();
#define REGISTER_INPUT(EVENT, FUNC_PTR) inputQueue->OnInputEvent.bind(EVENT, this->weak_from_this(), std::bind(FUNC_PTR, this, std::placeholders::_1))
	REGISTER_INPUT(input::EInputType::KEY, &PlayerInputComponent::onKeyInput);
	REGISTER_INPUT(input::EInputType::MOUSE_MOVE, &PlayerInputComponent::onMouseMove);
#undef REGISTER_INPUT
}

void PlayerInputComponent::onKeyInput(input::Event const & evt)
{
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

void PlayerInputComponent::onMouseMove(input::Event const & evt)
{
	this->mLookHorizontal.delta = evt.inputMouseMove.xDelta;
	this->mLookVertical.delta = evt.inputMouseMove.yDelta;
}

f32 const& PlayerInputComponent::axialMoveSpeed() const
{
	return this->mAxialMoveSpeed;
}

std::array<PlayerInputComponent::InputMapping, 6> const& PlayerInputComponent::axialMoveMappings() const
{
	return this->mAxisMappings;
}

std::vector<PlayerInputComponent::InputAxis*> PlayerInputComponent::lookAxes()
{
	return { &this->mLookVertical, &this->mLookHorizontal };
}
