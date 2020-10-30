#include "render/VoxelGridRenderer.hpp"

#include "Model.hpp"
#include "StitchedTexture.hpp"
#include "asset/PipelineAsset.hpp"
#include "asset/Shader.hpp"
#include "graphics/Command.hpp"
#include "graphics/Pipeline.hpp"

#include "registry/VoxelType.hpp"
#include "render/VoxelModelManager.hpp"
#include "world/BlockInstanceMap.hpp"

using namespace graphics;

VoxelGridRenderer::VoxelGridRenderer()
{
	this->mDescriptorPool.setPoolSize(64, {
		{ vk::DescriptorType::eUniformBuffer, 64 },
		{ vk::DescriptorType::eCombinedImageSampler, 64 },
	});
	this->mpBlockRenderInstances = std::make_shared<world::BlockInstanceMap>();
}

VoxelGridRenderer::~VoxelGridRenderer()
{
	destroyRenderDevices();
}

void VoxelGridRenderer::destroyRenderDevices()
{
	this->mpBlockRenderInstances.reset();
}

VoxelGridRenderer& VoxelGridRenderer::setPipeline(std::shared_ptr<asset::Pipeline> asset)
{
	if (this->mpPipeline)
	{
		this->mpPipeline.reset();
	}
	this->mpPipeline = std::make_shared<graphics::Pipeline>();

	// TODO: offset and size will need to be scaled by current frame resolution
	this->mpPipeline->addViewArea(asset->getViewport(), asset->getScissor());
	this->mpPipeline->setBlendMode(asset->getBlendMode());
	this->mpPipeline->setFrontFace(asset->getFrontFace());

	// Perform a synchronous load on each shader to create the shader modules
	this->mpPipeline->addShader(asset->getVertexShader().load(asset::EAssetSerialization::Binary)->makeModule());
	this->mpPipeline->addShader(asset->getFragmentShader().load(asset::EAssetSerialization::Binary)->makeModule());

	{
		ui8 slot = 0;
		auto bindings = Model::bindings(slot);
		bindings.push_back(world::BlockInstanceMap::getBinding(slot));
		this->mpPipeline->setBindings(bindings);
	}

	// Create descriptor groups for the pipeline
	for (auto const& assetDescGroup : asset->getDescriptorGroups())
	{
		auto const& descriptors = assetDescGroup.descriptors;
		auto descriptorGroup = graphics::DescriptorGroup();
		descriptorGroup.setBindingCount(descriptors.size());
		for (uIndex i = 0; i < descriptors.size(); ++i)
		{
			descriptorGroup.addBinding(descriptors[i].id, i, descriptors[i].type, descriptors[i].stage);
		}
		this->mDescriptorGroups.push_back(std::move(descriptorGroup));
	}

	return *this;
}

void VoxelGridRenderer::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	this->mpPipeline->setDevice(device);
	this->mDescriptorPool.setDevice(device);

	this->mpBlockRenderInstances->setDevice(device);
	this->mpBlockRenderInstances->constructInstanceBuffer(6, 16);
}

void VoxelGridRenderer::setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass)
{
	this->mpPipeline->setRenderPass(renderPass);
}

void VoxelGridRenderer::createVoxelDescriptorMapping(
	std::shared_ptr<game::VoxelTypeRegistry> registry,
	std::shared_ptr<game::VoxelModelManager> modelManager
)
{
	this->mpTypeRegistry = registry;
	this->mpModelManager = modelManager;
	this->mDescriptorGroups[1].setArchetypeAmount(modelManager->getAtlasCount());
	
	for (auto const& idPath : registry->getEntriesById())
	{
		this->mVoxelIdToDescriptorArchetype.insert(std::make_pair(idPath.first, modelManager->getAtlasIndex(idPath.first)));
	}
}

void VoxelGridRenderer::setFrameCount(uSize frameCount)
{
	this->mDescriptorPool.setAllocationMultiplier((ui32)frameCount);
	this->mDescriptorGroups[0].setAmount((ui32)frameCount);
	this->mDescriptorGroups[1].setAmount(1);
}

void VoxelGridRenderer::createDescriptors(std::shared_ptr<graphics::GraphicsDevice> device)
{
	this->mDescriptorPool.create();
	for (auto& descriptorGroup : this->mDescriptorGroups)
	{
		descriptorGroup.create(device, &this->mDescriptorPool);
	}
}

void VoxelGridRenderer::attachDescriptors(
	std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
)
{
	this->mDescriptorGroups[0].attachToBinding("mvpUniform", mutableUniforms["mvpUniform"]);

	auto modelManager = this->mpModelManager.lock();
	for (uIndex idxAtlas = 0; idxAtlas < modelManager->getAtlasCount(); ++idxAtlas)
	{
		auto atlas = modelManager->getAtlas(idxAtlas);
		this->mDescriptorGroups[1].attachToBinding(
			"imageAtlas", vk::ImageLayout::eShaderReadOnlyOptimal,
			atlas->view(), modelManager->getSampler().get(),
			/*idxArchetype=*/ idxAtlas
		);
	}
}

void VoxelGridRenderer::writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device)
{
	for (auto& descriptorGroup : this->mDescriptorGroups)
	{
		descriptorGroup.writeAttachments(device);
	}
}

void VoxelGridRenderer::createPipeline(math::Vector2UInt const& resolution)
{
	this->mpPipeline
		->setDescriptors(&this->mDescriptorGroups)
		.setResolution(resolution)
		.create();
}

void VoxelGridRenderer::writeInstanceBuffer(graphics::CommandPool* transientPool)
{
	this->mpBlockRenderInstances->writeInstanceBuffer(transientPool);
}

void VoxelGridRenderer::record(graphics::Command *command, uIndex idxFrame)
{
	OPTICK_EVENT();
	// TODO: Optimize draw calls by only re-recording if the instance buffer has changed since the last time this frame's command buffer was recorded

	auto registry = this->mpTypeRegistry.lock();
	auto modelManager = this->mpModelManager.lock();
	auto id = game::BlockId("minecraft", "grass");
	//for (auto const& idPath : registry->getEntriesById())
	{
		//auto id = idPath.first;
		OPTICK_GPU_EVENT("DrawVoxel");
		OPTICK_TAG("VoxelId", id.to_string().c_str());

		// TODO: Can optimize by iterating over this map and moving the frame descriptor fetch outside the loop
		auto atlasIter = this->mVoxelIdToDescriptorArchetype.find(id);
		assert(atlasIter != this->mVoxelIdToDescriptorArchetype.end());
		auto descriptorAtlasArchetypeIdx = atlasIter->second;

		auto descriptorSets = std::vector<vk::DescriptorSet>();
		descriptorSets.push_back(this->mDescriptorGroups[0].getDescriptorSet(idxFrame));
		descriptorSets.push_back(this->mDescriptorGroups[1].getDescriptorSet(0, descriptorAtlasArchetypeIdx));
		command->bindDescriptorSets(this->mpPipeline, descriptorSets);
		command->bindPipeline(this->mpPipeline);

		auto profile = modelManager->getBufferProfile(id);
		command->bindVertexBuffers(0, { profile.vertexBuffer });
		command->bindIndexBuffer(0, profile.indexBuffer, vk::IndexType::eUint16);

		auto instanceData = this->mpBlockRenderInstances->getBlockInstanceData(id);
		command->bindVertexBuffers(1, { instanceData.buffer });
		
		command->draw(
			profile.indexBufferStartIndex,
			profile.indexBufferCount,
			profile.indexBufferValueOffset,
			instanceData.offset, instanceData.count
		);
		//break; // because the instance data doesnt have specifics for each block id yet
	}

}

void VoxelGridRenderer::destroyRenderChain()
{
	this->mpPipeline->invalidate();
	for (auto& descriptorGroup : this->mDescriptorGroups)
	{
		descriptorGroup.invalidate();
	}
	this->mDescriptorGroups.clear();
	this->mDescriptorPool.invalidate();
	this->mVoxelIdToDescriptorArchetype.clear();
}
