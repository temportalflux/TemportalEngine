#include "render/VoxelGridRenderer.hpp"

#include "Model.hpp"
#include "StitchedTexture.hpp"
#include "asset/PipelineAsset.hpp"
#include "asset/Shader.hpp"
#include "graphics/Command.hpp"
#include "graphics/Pipeline.hpp"
#include "graphics/DescriptorPool.hpp"
#include "graphics/assetHelpers.hpp"

#include "registry/VoxelType.hpp"
#include "render/VoxelModelManager.hpp"
#include "world/BlockInstanceMap.hpp"

using namespace graphics;

VoxelGridRenderer::VoxelGridRenderer(
	std::weak_ptr<graphics::DescriptorPool> pDescriptorPool,
	std::weak_ptr<world::BlockInstanceBuffer> instanceBuffer,
	std::weak_ptr<game::VoxelTypeRegistry> registry,
	std::weak_ptr<game::VoxelModelManager> modelManager
)
{
	this->mpDescriptorPool = pDescriptorPool;
	this->mpInstanceBuffer = instanceBuffer;
	this->mpTypeRegistry = registry;
	this->mpModelManager = modelManager;
}

VoxelGridRenderer::~VoxelGridRenderer()
{
	destroy();
}

void VoxelGridRenderer::destroy()
{
	this->mAtlasDescriptors.clear();
	this->mDescriptorLayoutUniform.invalidate();
	this->mDescriptorLayoutTexture.invalidate();
}

void fillDescriptorLayout(graphics::DescriptorLayout &layout, std::vector<asset::Pipeline::Descriptor> const& descriptors)
{
	layout.setBindingCount(descriptors.size());
	for (uIndex i = 0; i < descriptors.size(); ++i)
	{
		layout.setBinding(
			i, descriptors[i].id,
			descriptors[i].type, descriptors[i].stage, 1
		);
	}
}

VoxelGridRenderer& VoxelGridRenderer::setPipeline(asset::TypedAssetPath<asset::Pipeline> const& path)
{
	if (!this->mpPipeline)
	{
		this->mpPipeline = std::make_shared<graphics::Pipeline>();
	}

	graphics::populatePipeline(path, this->mpPipeline.get(), nullptr);

	auto const& descrGroups = path.load(asset::EAssetSerialization::Binary)->getDescriptorGroups();
	fillDescriptorLayout(this->mDescriptorLayoutUniform, descrGroups[0].descriptors);
	fillDescriptorLayout(this->mDescriptorLayoutTexture, descrGroups[1].descriptors);

	{
		ui8 slot = 0;
		auto bindings = Model::bindings(slot);
		bindings.push_back(world::BlockInstanceBuffer::getBinding(slot));
		this->mpPipeline->setBindings(bindings);
	}

	return *this;
}

void VoxelGridRenderer::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	this->mpPipeline->setDevice(device);
	this->mDescriptorLayoutUniform.setDevice(device);
	this->mDescriptorLayoutUniform.create();
	this->mDescriptorLayoutTexture.setDevice(device);
	this->mDescriptorLayoutTexture.create();
}

void VoxelGridRenderer::setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass)
{
	this->mpPipeline->setRenderPass(renderPass);
}

void VoxelGridRenderer::setFrameCount(uSize frameCount)
{
	this->mUniformDescriptors.resize(frameCount);
}

void VoxelGridRenderer::createDescriptors(std::shared_ptr<graphics::GraphicsDevice> device)
{
	this->mDescriptorLayoutUniform.createSets(this->mpDescriptorPool.lock().get(), this->mUniformDescriptors);
}

void VoxelGridRenderer::attachDescriptors(
	std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
)
{
	for (uIndex idxSet = 0; idxSet < this->mUniformDescriptors.size(); ++idxSet)
	{
		this->mUniformDescriptors[idxSet].attach("mvpUniform", mutableUniforms["cameraUniform"][idxSet]);
	}
}

void VoxelGridRenderer::writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device)
{
	for (auto& set : this->mUniformDescriptors) set.writeAttachments();
}

void VoxelGridRenderer::createPipeline(math::Vector2UInt const& resolution)
{
	this->mpPipeline
		->setDescriptorLayouts({ &this->mDescriptorLayoutUniform, &this->mDescriptorLayoutTexture })
		.setResolution(resolution)
		.create();
}

std::function<void()> VoxelGridRenderer::onAtlasesCreatedEvent()
{
	return std::bind(&VoxelGridRenderer::onAtlasesCreated, this);
}

void VoxelGridRenderer::onAtlasesCreated()
{
	auto modelManager = this->mpModelManager.lock();
	this->mAtlasDescriptors.resize(modelManager->getAtlasCount());
	this->mDescriptorLayoutTexture.createSets(this->mpDescriptorPool.lock().get(), this->mAtlasDescriptors);

	for (auto const& idPath : this->mpTypeRegistry.lock()->getEntriesById())
	{
		this->mDescriptorSetIdxByVoxelId.insert(std::make_pair(idPath.first, modelManager->getAtlasIndex(idPath.first)));
	}

	for (uIndex idxAtlas = 0; idxAtlas < modelManager->getAtlasCount(); ++idxAtlas)
	{
		auto atlas = modelManager->getAtlas(idxAtlas);
		this->mAtlasDescriptors[idxAtlas].attach(
			"imageAtlas", graphics::EImageLayout::eShaderReadOnlyOptimal,
			atlas->view(), modelManager->getSampler().get()
		).writeAttachments();
	}
}

void VoxelGridRenderer::record(graphics::Command *command, uIndex idxFrame)
{
	OPTICK_EVENT();
	// TODO: Optimize draw calls by only re-recording if the instance buffer has changed since the last time this frame's command buffer was recorded

	auto registry = this->mpTypeRegistry.lock();
	auto modelManager = this->mpModelManager.lock();
	auto instanceBuffer = this->mpInstanceBuffer.lock();
	//auto id = game::BlockId("minecraft", "grass");
	for (auto const& idPath : registry->getEntriesById())
	{
		auto id = idPath.first;
		auto instanceData = instanceBuffer->getDataForVoxelId(id);
		if (instanceData.count == 0) continue;

		OPTICK_GPU_EVENT("DrawVoxel");
		OPTICK_TAG("VoxelId", id.to_string().c_str());

		// TODO: Can optimize by iterating over this map and moving the frame descriptor fetch outside the loop
		auto descriptorIdxIter = this->mDescriptorSetIdxByVoxelId.find(id);
		assert(descriptorIdxIter != this->mDescriptorSetIdxByVoxelId.end());

		auto descriptorSets = std::vector<graphics::DescriptorSet*>();
		descriptorSets.push_back(&this->mUniformDescriptors[idxFrame]);
		descriptorSets.push_back(&this->mAtlasDescriptors[descriptorIdxIter->second]);
		command->bindDescriptorSets(this->mpPipeline, descriptorSets);
		command->bindPipeline(this->mpPipeline);

		auto profile = modelManager->getBufferProfile(id);
		command->bindVertexBuffers(0, { profile.vertexBuffer });
		command->bindIndexBuffer(0, profile.indexBuffer, vk::IndexType::eUint16);

		command->bindVertexBuffers(1, { instanceData.buffer });
		
		command->draw(
			profile.indexBufferStartIndex,
			profile.indexBufferCount,
			profile.indexBufferValueOffset,
			(ui32)instanceData.index, (ui32)instanceData.count
		);
	}

}

void VoxelGridRenderer::destroyRenderChain()
{
	this->mpPipeline->invalidate();
	this->mUniformDescriptors.clear();
}
