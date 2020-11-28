#include "ecs/component/ComponentCameraPOV.hpp"

#include "Engine.hpp"
#include "input/Queue.hpp"

using namespace ecs;
using namespace ecs::component;

DEFINE_ECS_COMPONENT_STATICS(CameraPOV)

CameraPOV::CameraPOV()
	: mFOV(27.0f)
	, mNearPlane(0.01f)
	, mFarPlane(100.0f)
{
}

f32 const& CameraPOV::fov() const { return this->mFOV; }
f32 const& CameraPOV::nearPlane() const { return this->mNearPlane; }
f32 const& CameraPOV::farPlane() const { return this->mFarPlane; }
