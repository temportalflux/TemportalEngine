#include "ecs/system/SystemRenderPlayer.hpp"

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

#include "ecs/view/ViewRenderedPlayer.hpp"
#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentCameraPOV.hpp"
#include "ecs/component/ComponentPlayerModel.hpp"

using namespace ecs;
using namespace ecs::system;

RenderPlayer::RenderPlayer(
	std::weak_ptr<graphics::SkinnedModelManager> modelManager,
	std::weak_ptr<graphics::DescriptorPool> globalDescriptorPool
)
	: System(view::RenderedPlayer::TypeId)
	, mpModelManager(modelManager)
	, mpDescriptorPool(globalDescriptorPool)
{
}

RenderPlayer::~RenderPlayer()
{
	destroy();
}

RenderPlayer& RenderPlayer::setPipeline(asset::TypedAssetPath<asset::Pipeline> const& path)
{
	if (!this->mpPipeline)
	{
		this->mpPipeline = std::make_shared<graphics::Pipeline>();
	}

	graphics::populatePipeline(path, this->mpPipeline.get(), &this->mDescriptorLayout);

	{
		typedef graphics::EntityInstanceBuffer::InstanceData TInstance;
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

void RenderPlayer::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	this->mpPipeline->setDevice(device);
	this->mDescriptorLayout.setDevice(device);
}

void RenderPlayer::setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass)
{
	this->mpPipeline->setRenderPass(renderPass);
}

void RenderPlayer::initializeData(graphics::CommandPool* transientPool)
{
	this->mDescriptorLayout.create();
}

void RenderPlayer::setFrameCount(uSize frameCount)
{
	this->mDescriptorSets.resize(frameCount);
}

void RenderPlayer::createDescriptors(std::shared_ptr<graphics::GraphicsDevice> device)
{
	this->mDescriptorLayout.createSets(this->mpDescriptorPool.lock().get(), this->mDescriptorSets);
}

void RenderPlayer::createPipeline(math::Vector2UInt const& resolution)
{
	this->mpPipeline
		->setDescriptorLayout(this->mDescriptorLayout, this->mDescriptorSets.size())
		.setResolution(resolution)
		.create();
}

void RenderPlayer::attachDescriptors(
	std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
)
{
	auto registry = game::Game::Get()->textureRegistry();
	for (uIndex idxSet = 0; idxSet < this->mDescriptorSets.size(); ++idxSet)
	{
		this->mDescriptorSets[idxSet].attach("cameraUniform", mutableUniforms["mvpUniform"][idxSet]);
		this->mDescriptorSets[idxSet].attach(
			"texture", graphics::EImageLayout::eShaderReadOnlyOptimal,
			registry->getImage(asset::SKIN_DEFAULT_MASCULINE).lock().get(),
			registry->getSampler(asset::SAMPLER_NEAREST_NEIGHBOR).lock().get()
		);
	}
}

void RenderPlayer::writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device)
{
	for (auto& descriptorSet : this->mDescriptorSets)
	{
		descriptorSet.writeAttachments();
	}
}

void RenderPlayer::update(f32 deltaTime, std::shared_ptr<ecs::view::View> view)
{
	OPTICK_EVENT();

	auto transform = view->get<component::CoordinateTransform>();
	auto playerModel = view->get<component::PlayerModel>();
	assert(transform && playerModel);

	graphics::EntityInstanceBuffer::InstanceData instance;
	instance.posOfCurrentChunk = transform->position().chunk().toFloat();
	instance.localTransform = math::createModelMatrix(
		{ 8, 5, 8 },
		//transform->position().local().toFloat() + transform->position().offset(),
		math::Quaternion::Identity,
		//math::Quaternion::FromAxisAngle(math::Vector3unitY, transform->orientation().euler().y()),
		transform->size()
	);
	playerModel->instanceBuffer()->markInstanceForUpdate(playerModel->instanceHandle(), instance);
}

void RenderPlayer::record(graphics::Command *command, uIndex idxFrame)
{
	OPTICK_EVENT();
	auto& ecs = engine::Engine::Get()->getECS();
	for (auto pView : ecs.views().getAllOfType(this->viewId()))
	{
		if (!pView->hasAllComponents()) continue;
		this->recordView(command, idxFrame, pView);
	}
}

void RenderPlayer::recordView(graphics::Command *command, uIndex idxFrame, std::shared_ptr<ecs::view::View> view)
{
	OPTICK_EVENT();

	auto cameraPOV = view->get<component::CameraPOV>();
	auto playerModel = view->get<component::PlayerModel>();
	assert(cameraPOV && playerModel);

	auto modelManager = this->mpModelManager.lock();
	command->bindDescriptorSets(this->mpPipeline, { &this->mDescriptorSets[idxFrame] });
	command->bindPipeline(this->mpPipeline);
	modelManager->bindBuffers(playerModel->modelHandle(), command);
	command->bindVertexBuffers(1, { playerModel->instanceBuffer()->buffer() });
	command->draw(0, modelManager->indexCount(playerModel->modelHandle()), 0, (ui32)playerModel->instanceHandle(), 1);
}

void RenderPlayer::destroyRenderChain()
{
	this->mpPipeline->invalidate();
	this->mDescriptorSets.clear();
}

void RenderPlayer::destroy()
{
	this->mDescriptorLayout.invalidate();
}
