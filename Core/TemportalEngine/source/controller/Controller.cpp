#include "controller/Controller.hpp"

#include "Engine.hpp"
#include "input/Queue.hpp"
#include "logging/Logger.hpp"
#include "ecs/component/ComponentTransform.hpp"
#include "glm/gtc/quaternion.hpp"

Controller::Controller()
{
	f32 moveSpeed = 4.0f;
	// TODO: These are using y up directions (it should really be Z up)
	this->mForward = { math::Vector3unitZ, false, moveSpeed };
	this->mBackward = { -math::Vector3unitZ, false, moveSpeed };
	this->mStrafeRight = { math::Vector3unitX, false, moveSpeed };
	this->mStrafeLeft = { -math::Vector3unitX, false, moveSpeed };
	this->mUp = { math::Vector3unitY, true, moveSpeed };
	this->mDown = { -math::Vector3unitY, true, moveSpeed };
	this->mInputMappings = {
		{ input::EKey::W, &this->mForward },
		{ input::EKey::S, &this->mBackward },
		{ input::EKey::A, &this->mStrafeLeft },
		{ input::EKey::D, &this->mStrafeRight },
		{ input::EKey::E, &this->mUp },
		{ input::EKey::Q, &this->mDown },
	};
	this->mLookHorizontal = { math::Vector3unitY, glm::radians(90.0f) };
	this->mLookVertical = { math::Vector3unitX, glm::radians(90.0f) };
}

void Controller::assignCameraTransform(ecs::component::Transform *transform)
{
	this->mpCameraTransform = transform;
}

void Controller::subscribeToInput()
{
	auto inputQueue = engine::Engine::Get()->getInputQueue();
#define REGISTER_INPUT(EVENT, FUNC_PTR) inputQueue->OnInputEvent.bind(EVENT, this->weak_from_this(), std::bind(FUNC_PTR, this, std::placeholders::_1))
	REGISTER_INPUT(input::EInputType::KEY, &Controller::onKeyInput);
	REGISTER_INPUT(input::EInputType::MOUSE_MOVE, &Controller::onMouseMove);
#undef REGISTER_INPUT
}

void Controller::onKeyInput(input::Event const & evt)
{
	auto mapping = this->mInputMappings.find(evt.inputKey.key);
	if (mapping != this->mInputMappings.end())
	{
		if (evt.inputKey.action == input::EAction::PRESS)
			mapping->second->bIsActive = true;
		if (evt.inputKey.action == input::EAction::RELEASE)
			mapping->second->bIsActive = false;
	}
}

void Controller::onMouseMove(input::Event const & evt)
{
	this->mLookHorizontal.delta = evt.inputMouseMove.xDelta;
	this->mLookVertical.delta = evt.inputMouseMove.yDelta;
}

void Controller::tick(f32 deltaTime)
{
	OPTICK_EVENT();
	static logging::Logger ControllerLog = DeclareLog("Controller");
	
	auto orientation = this->mpCameraTransform->orientation;
	auto euler = orientation.euler();
	auto rot = math::Quaternion::FromAxisAngle(math::Vector3unitY, euler.y());
	
	for (auto&[key, mapping] : this->mInputMappings)
	{
		if (!mapping->bIsActive) continue;
		auto dir = mapping->bIsGlobal ? mapping->direction : rot.rotate(mapping->direction);
		this->mpCameraTransform->move(dir * deltaTime * mapping->speed);
	}

	/* NOTE: on rotational drift
		To account for multi-axis rotations, the axes are applied as different orders of concatenation
		See: https://gamedev.stackexchange.com/a/136175
	*/

	// TODO: This isn't really frame independent (doesn't use deltaTime)
	if (std::abs(this->mLookVertical.delta) > std::numeric_limits<f32>::epsilon())
	{
		orientation = math::Quaternion::concat(orientation, math::Quaternion::FromAxisAngle(
			this->mLookVertical.axis, this->mLookVertical.radians * this->mLookVertical.delta
		));
		this->mLookVertical.delta = 0.0f;
	}

	if (std::abs(this->mLookHorizontal.delta) > std::numeric_limits<f32>::epsilon())
	{
		orientation = math::Quaternion::concat(math::Quaternion::FromAxisAngle(
			this->mLookHorizontal.axis, this->mLookHorizontal.radians * this->mLookHorizontal.delta
		), orientation);
		this->mLookHorizontal.delta = 0.0f;
	}
	
	this->mpCameraTransform->orientation = orientation;
}
