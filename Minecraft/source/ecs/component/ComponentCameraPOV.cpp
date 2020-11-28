#include "ecs/component/ComponentCameraPOV.hpp"

#include "Engine.hpp"
#include "input/Queue.hpp"

using namespace ecs;
using namespace ecs::component;

DEFINE_ECS_COMPONENT_STATICS(CameraPOV)

CameraPOV::CameraPOV()
{
	this->mFOV = 45.0f;
}

f32 const& CameraPOV::fov() const
{
	return this->mFOV;
}
