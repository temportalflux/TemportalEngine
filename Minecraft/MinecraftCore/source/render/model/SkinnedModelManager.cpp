#include "render/model/SkinnedModelManager.hpp"

#include "asset/ModelAsset.hpp"
#include "graphics/Command.hpp"

using namespace graphics;

SkinnedModel::SkinnedModel()
{
	this->mVertexBuffer.setUsage(
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		graphics::MemoryUsage::eGPUOnly
	);
	this->mIndexBuffer.setUsage(
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
		graphics::MemoryUsage::eGPUOnly
	);
}

SkinnedModel& SkinnedModel::setBase(std::vector<ModelVertex> const& vertices, std::vector<ui32> const& indices)
{
	this->mVertices = vertices;
	this->mIndices = indices;
	return *this;
}

SkinnedModel& SkinnedModel::setBase(std::shared_ptr<asset::Model> const& model)
{
	return this->setBase(model->vertices(), model->indices());
}

void SkinnedModel::setDevice(std::weak_ptr<GraphicsDevice> device)
{
	this->mVertexBuffer.setDevice(device);
	this->mIndexBuffer.setDevice(device);
}

void* SkinnedModel::vertexBufferData() { return this->mVertices.data(); }
uSize SkinnedModel::vertexBufferSize() const { return this->mVertices.size() * sizeof(ModelVertex); }
void* SkinnedModel::indexBufferData() { return this->mIndices.data(); }
uSize SkinnedModel::indexBufferSize() const { return this->mIndices.size() * sizeof(ui32); }

void SkinnedModel::create()
{
	this->mVertexBuffer.setSize(this->vertexBufferSize()).create();
	this->mIndexBuffer.setSize(this->indexBufferSize()).create();
}

void SkinnedModel::initializeBuffers(graphics::CommandPool* transientPool)
{
	// TODO: These can be done in one operation, and we don't need to wait for the graphics device to be done (nothing relies on this process except starting rendering)
	this->mVertexBuffer.writeBuffer(transientPool, 0, this->vertexBufferData(), this->vertexBufferSize());
	this->mIndexBuffer.writeBuffer(transientPool, 0, this->indexBufferData(), this->indexBufferSize());
}

void SkinnedModel::updateVertexBuffer(graphics::CommandPool* transientPool)
{
	this->mVertexBuffer.writeBuffer(transientPool, 0, this->vertexBufferData(), this->vertexBufferSize());
}

void SkinnedModel::invalidate()
{
	this->mVertexBuffer.invalidate();
	this->mVertexBuffer.setSize(0);
	this->mIndexBuffer.invalidate();
	this->mIndexBuffer.setSize(0);
}

void SkinnedModel::destroy()
{
	this->mVertexBuffer.destroy();
	this->mIndexBuffer.destroy();
}

SkinnedModelManager::SkinnedModelManager(
	std::weak_ptr<GraphicsDevice> device,
	graphics::CommandPool* transientPool
) : mpDevice(device), mpTransientCmdPool(transientPool)
{
}

SkinnedModelManager::~SkinnedModelManager()
{
	for (auto& model : this->mModels) model.destroy();
}

SkinnedModel& SkinnedModelManager::createModel(Handle &outHandle)
{
	if (this->mUnusedHandles.size() > 0)
	{
		auto iter = this->mUnusedHandles.begin();
		outHandle = *iter;
		this->mUnusedHandles.erase(iter);
	}
	else
	{
		outHandle = this->mModels.size();
		auto model = SkinnedModel();
		model.setDevice(this->mpDevice);
		this->mModels.push_back(std::move(model));
	}
	return this->mModels[outHandle];
}

SkinnedModelManager::Handle SkinnedModelManager::createAssetModel(std::shared_ptr<asset::Model> asset)
{
	Handle handle;
	SkinnedModel& model = this->createModel(handle);
	model.setBase(asset).create();
	model.initializeBuffers(this->mpTransientCmdPool);
	return handle;
}

void SkinnedModelManager::destroyModel(Handle const& validHandle)
{
	this->mModels[validHandle].invalidate();
	this->mUnusedHandles.insert(validHandle);
}

void SkinnedModelManager::bindBuffers(Handle const& validHandle, graphics::Command *command)
{
	auto& model = this->mModels[validHandle];
	command->bindVertexBuffers(0, { &model.mVertexBuffer });
	command->bindIndexBuffer(0, & model.mIndexBuffer, vk::IndexType::eUint32);
}

ui32 SkinnedModelManager::indexCount(Handle const& validHandle) const
{
	return (ui32)this->mModels[validHandle].mIndices.size();
}
