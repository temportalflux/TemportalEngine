#include "ecs/component/ComponentCameraPOV.hpp"

#include "Engine.hpp"
#include "input/Queue.hpp"

using namespace ecs;
using namespace ecs::component;

DEFINE_ECS_COMPONENT_STATICS(CameraPOV)

CameraPOV::CameraPOV()
	: mOffset({ 0, 0, 0 })
	, mFOV(27.0f)
	, mNearPlane(0.01f)
	, mFarPlane(100.0f)
{
}

CameraPOV& CameraPOV::setFOV(f32 verticalFOV)
{
	this->mFOV = verticalFOV;
	return *this;
}

math::Vector3 const& CameraPOV::offset() const { return this->mOffset; }
f32 const& CameraPOV::fov() const { return this->mFOV; }
f32 const& CameraPOV::nearPlane() const { return this->mNearPlane; }
f32 const& CameraPOV::farPlane() const { return this->mFarPlane; }
