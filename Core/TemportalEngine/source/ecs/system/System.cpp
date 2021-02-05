#include "ecs/system/System.hpp"

#include "Engine.hpp"
#include "ecs/Core.hpp"

using namespace evcs;
using namespace evcs::system;

System::System(ViewTypeId const& viewTypeId)
	: mViewTypeId(viewTypeId)
{
}

ViewTypeId const& System::viewId() const
{
	return this->mViewTypeId;
}

void System::tick(f32 deltaTime)
{
	auto& ecs = engine::Engine::Get()->getECS();
	for (auto pView : ecs.views().getAllOfType(this->mViewTypeId))
	{
		if (!pView->hasAllComponents()) continue;
		this->update(deltaTime, pView);
	}
}
