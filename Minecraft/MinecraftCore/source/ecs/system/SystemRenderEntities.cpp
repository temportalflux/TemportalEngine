#include "ecs/system/SystemRenderEntities.hpp"

#include "Engine.hpp"
#include "ecs/Core.hpp"
#include "Game.hpp"

#include "asset/MinecraftAssetStatics.hpp"
#include "graphics/assetHelpers.hpp"
#include "graphics/Pipeline.hpp"
#include "graphics/Command.hpp"
#include "render/model/SkinnedModelManager.hpp"
#include "render/EntityInstanceBuffer.hpp"
#include "render/TextureRegistry.hpp"
#include "render/ModelVertex.hpp"

#include "ecs/view/ViewRenderedMesh.hpp"
#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentRenderMesh.hpp"

using namespace ecs;
using namespace ecs::system;

RenderEntities::RenderEntities()
	: System(view::RenderedMesh::TypeId)
{
}

RenderEntities::~RenderEntities()
{
	destroy();
}

RenderEntities& RenderEntities::setPipeline(asset::TypedAssetPath<asset::Pipeline> const& path)
{
	if (!this->mpPipeline)
	{
		this->mpPipeline = std::make_shared<graphics::Pipeline>();
	}

	graphics::populatePipeline(path, this->mpPipeline.get(), nullptr);

	{
		typedef graphics::EntityInstanceData TInstance;
		ui8 slot = 0;
		this->mpPipeline->setBindings({
			graphics::AttributeBinding(graphics::AttributeBinding::Rate::eVertex)
				.setStructType<ModelVertex>()
				.addAttribute({ slot++, /*vec3*/(ui32)vk::Format::eR32G32B32Sfloat, offsetof(ModelVertex, position) })
				.addAttribute({ slot++, /*vec3*/(ui32)vk::Format::eR32G32B32Sfloat, offsetof(ModelVertex, normal) })
				.addAttribute({ slot++, /*vec3*/(ui32)vk::Format::eR32G32B32Sfloat, offsetof(ModelVertex, tangent) })
				.addAttribute({ slot++, /*vec3*/(ui32)vk::Format::eR32G32B32Sfloat, offsetof(ModelVertex, bitangent) })
				.addAttribute({ slot++, /*vec2*/(ui32)vk::Format::eR32G32Sfloat, offsetof(ModelVertex, texCoord) }),
			graphics::AttributeBinding(graphics::AttributeBinding::Rate::eInstance)
				.setStructType<TInstance>()
				.addAttribute({ slot++, /*vec3*/(ui32)vk::Format::eR32G32B32Sfloat, offsetof(TInstance, posOfCurrentChunk) })
				// mat4 using 4 slots
				.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(TInstance, localTransform) + (0 * sizeof(math::Vector4)) })
				.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(TInstance, localTransform) + (1 * sizeof(math::Vector4)) })
				.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(TInstance, localTransform) + (2 * sizeof(math::Vector4)) })
				.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(TInstance, localTransform) + (3 * sizeof(math::Vector4)) })
		});
	}

	return *this;
}

void RenderEntities::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	this->mpPipeline->setDevice(device);
}

void RenderEntities::setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass)
{
	this->mpPipeline->setRenderPass(renderPass);
}

void RenderEntities::initializeData(graphics::CommandPool* transientPool, graphics::DescriptorPool *descriptorPool)
{
}

void RenderEntities::createLocalPlayerDescriptor()
{
}

void RenderEntities::setFrameCount(uSize frameCount)
{
}

void RenderEntities::createDescriptors(graphics::DescriptorPool *descriptorPool)
{
}

void RenderEntities::setDescriptorLayouts(std::unordered_map<std::string, graphics::DescriptorLayout const*> const& globalLayouts)
{
	auto registry = game::Game::Get()->textureRegistry();
	this->mpPipeline->setDescriptorLayouts({ globalLayouts.find("camera")->second, registry->textureLayout() });
}

void RenderEntities::createPipeline(math::Vector2UInt const& resolution)
{
	this->mpPipeline->setResolution(resolution).create();
}

void RenderEntities::attachDescriptors(
	std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
)
{
}

void RenderEntities::writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device)
{
}

void RenderEntities::update(f32 deltaTime, std::shared_ptr<ecs::view::View> view)
{
	OPTICK_EVENT();

	auto transform = view->get<component::CoordinateTransform>();
	auto renderMesh = view->get<component::RenderMesh>();
	assert(transform && renderMesh);

	auto* instance = renderMesh->instanceHandle().get();
	instance->posOfCurrentChunk = transform->position().chunk().toFloat();
	instance->localTransform = math::createModelMatrix(
		transform->position().local().toFloat() + transform->position().offset(),
		math::Quaternion::FromAxisAngle(math::V3_UP, transform->orientation().euler().y()),
		transform->size()
	);
	renderMesh->instanceHandle().markDirty();
}

void RenderEntities::record(graphics::Command *command, uIndex idxFrame, TGetGlobalDescriptorSet getGlobalDescriptorSet)
{
	OPTICK_EVENT();
	auto& ecs = engine::Engine::Get()->getECS();
	auto pCameraDescSet = getGlobalDescriptorSet("camera", idxFrame);
	for (auto pView : ecs.views().getAllOfType(this->viewId()))
	{
		if (!pView->hasAllComponents()) continue;
		this->recordView(command, pCameraDescSet, pView);
	}
}

void RenderEntities::recordView(graphics::Command *command, graphics::DescriptorSet const* cameraSet, std::shared_ptr<ecs::view::View> view)
{
	OPTICK_EVENT();

	auto renderMesh = view->get<component::RenderMesh>();
	assert(renderMesh);

	auto const& textureDescriptor = game::Game::Get()->textureRegistry()->getDescriptorHandle(renderMesh->textureId());
	
	command->bindDescriptorSets(this->mpPipeline, std::vector<graphics::DescriptorSet const*>{ cameraSet, textureDescriptor.get() });
	command->bindPipeline(this->mpPipeline);
	renderMesh->modelHandle().get()->bindBuffers(command);
	command->bindVertexBuffers(1, { renderMesh->instanceHandle().owner<graphics::EntityInstanceBuffer>()->buffer() });
	command->draw(0, renderMesh->modelHandle().get()->indexCount(), 0, ui32(uIndex(renderMesh->instanceHandle())), 1);
}

void RenderEntities::destroyRenderChain()
{
	this->mpPipeline->invalidate();
}

void RenderEntities::destroy()
{
}
