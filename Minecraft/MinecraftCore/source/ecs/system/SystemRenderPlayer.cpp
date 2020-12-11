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
	graphics::DescriptorPool *descriptorPool
)
	: System(view::RenderedPlayer::TypeId)
	, mpModelManager(modelManager)
	, mpModelDescriptors(std::make_shared<graphics::DescriptorSetPool>(descriptorPool))
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

	graphics::populatePipeline(path, this->mpPipeline.get(), &this->mpModelDescriptors->layout());

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
	this->mpModelDescriptors->layout().setDevice(device);
}

void RenderPlayer::setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass)
{
	this->mpPipeline->setRenderPass(renderPass);
}

void RenderPlayer::initializeData(graphics::CommandPool* transientPool, graphics::DescriptorPool *descriptorPool)
{
	this->mpModelDescriptors->layout().create();
	this->mModelDescriptor_DefaultHumanoid = this->mpModelDescriptors->createHandle();
}

void RenderPlayer::createLocalPlayerDescriptor()
{
	auto registry = game::Game::Get()->textureRegistry();
	this->mModelDescriptor_DefaultHumanoid.get().attach(
		"texture", graphics::EImageLayout::eShaderReadOnlyOptimal,
		registry->getImage("model:DefaultHumanoid").lock().get(),
		registry->getSampler(asset::SAMPLER_NEAREST_NEIGHBOR).lock().get()
	).writeAttachments();
}

void RenderPlayer::setFrameCount(uSize frameCount)
{
}

void RenderPlayer::createDescriptors(graphics::DescriptorPool *descriptorPool)
{
}

void RenderPlayer::setDescriptorLayouts(std::unordered_map<std::string, graphics::DescriptorLayout const*> const& globalLayouts)
{
	this->mpPipeline->setDescriptorLayouts({ globalLayouts.find("camera")->second, &this->mpModelDescriptors->layout() });
}

void RenderPlayer::createPipeline(math::Vector2UInt const& resolution)
{
	this->mpPipeline->setResolution(resolution).create();
}

void RenderPlayer::attachDescriptors(
	std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
)
{
}

void RenderPlayer::writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device)
{
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
		transform->position().local().toFloat() + transform->position().offset(),
		math::Quaternion::FromAxisAngle(math::V3_UP, transform->orientation().euler().y()),
		transform->size()
	);
	playerModel->instanceBuffer()->markInstanceForUpdate(playerModel->instanceHandle(), instance);
}

void RenderPlayer::record(graphics::Command *command, uIndex idxFrame, TGetGlobalDescriptorSet getGlobalDescriptorSet)
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

void RenderPlayer::recordView(graphics::Command *command, graphics::DescriptorSet const* cameraSet, std::shared_ptr<ecs::view::View> view)
{
	OPTICK_EVENT();

	auto cameraPOV = view->get<component::CameraPOV>();
	auto playerModel = view->get<component::PlayerModel>();
	assert(cameraPOV && playerModel);

	auto modelManager = this->mpModelManager.lock();
	command->bindDescriptorSets(this->mpPipeline, std::vector<graphics::DescriptorSet const*>{ cameraSet, &this->mModelDescriptor_DefaultHumanoid.get() });
	command->bindPipeline(this->mpPipeline);
	modelManager->bindBuffers(playerModel->modelHandle(), command);
	command->bindVertexBuffers(1, { playerModel->instanceBuffer()->buffer() });
	command->draw(0, modelManager->indexCount(playerModel->modelHandle()), 0, (ui32)playerModel->instanceHandle(), 1);
}

void RenderPlayer::destroyRenderChain()
{
	this->mpPipeline->invalidate();
}

void RenderPlayer::destroy()
{
	this->mModelDescriptor_DefaultHumanoid.destroy();
	this->mpModelDescriptors->layout().invalidate();
}
