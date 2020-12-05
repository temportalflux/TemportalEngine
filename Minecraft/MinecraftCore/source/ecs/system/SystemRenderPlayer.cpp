#include "ecs/system/SystemRenderPlayer.hpp"

#include "Engine.hpp"
#include "ecs/Core.hpp"

#include "render/model/SkinnedModelManager.hpp"

#include "ecs/view/ViewRenderedPlayer.hpp"
#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentCameraPOV.hpp"

using namespace ecs;
using namespace ecs::system;

RenderPlayer::RenderPlayer(
	std::weak_ptr<graphics::SkinnedModelManager> modelManager
)
: System(view::RenderedPlayer::TypeId)
, mpModelManager(modelManager)
{
}

void RenderPlayer::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{

}

void RenderPlayer::setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass)
{

}

void RenderPlayer::initializeData(graphics::CommandPool* transientPool)
{

}

void RenderPlayer::setFrameCount(uSize frameCount)
{

}

void RenderPlayer::createDescriptors(std::shared_ptr<graphics::GraphicsDevice> device)
{

}

void RenderPlayer::attachDescriptors(
	std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
)
{

}

void RenderPlayer::writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device)
{

}

void RenderPlayer::createPipeline(math::Vector2UInt const& resolution)
{

}

void RenderPlayer::record(graphics::Command *command, uIndex idxFrame)
{
	OPTICK_EVENT();
	auto& ecs = engine::Engine::Get()->getECS();
	for (auto pView : ecs.views().getAllOfType(this->viewId()))
	{
		if (!pView->hasAllComponents()) continue;
		this->recordView(command, pView);
	}
}

void RenderPlayer::recordView(graphics::Command *command, std::shared_ptr<ecs::view::View> view)
{
	OPTICK_EVENT();

	auto transform = view->get<component::CoordinateTransform>();
	auto cameraPOV = view->get<component::CameraPOV>();
	assert(transform && cameraPOV);

}

void RenderPlayer::destroyRenderChain()
{

}
