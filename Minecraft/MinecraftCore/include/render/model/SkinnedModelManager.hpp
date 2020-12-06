#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/Buffer.hpp"
#include "render/ModelVertex.hpp"

FORWARD_DEF(NS_ASSET, class Model);
FORWARD_DEF(NS_GRAPHICS, class CommandPool);
FORWARD_DEF(NS_GRAPHICS, class Command);

NS_GRAPHICS
class GraphicsDevice;

class SkinnedModel
{
	friend class SkinnedModelManager;

public:
	SkinnedModel();

	SkinnedModel& setBase(std::vector<ModelVertex> const& vertices, std::vector<ui32> const& indices);
	SkinnedModel& setBase(std::shared_ptr<asset::Model> const& model);

	void* vertexBufferData();
	uSize vertexBufferSize() const;
	void* indexBufferData();
	uSize indexBufferSize() const;

	void setDevice(std::weak_ptr<GraphicsDevice> device);
	void create();
	void initializeBuffers(graphics::CommandPool* transientPool);
	void updateVertexBuffer(graphics::CommandPool* transientPool);
	void invalidate();
	void destroy();

private:
	std::vector<ModelVertex> mVertices;
	std::vector<ui32> mIndices;

	graphics::Buffer mVertexBuffer;
	graphics::Buffer mIndexBuffer;

};

class SkinnedModelManager
{
public:
	typedef uIndex Handle;

	SkinnedModelManager(
		std::weak_ptr<GraphicsDevice> device,
		graphics::CommandPool* transientPool
	);
	~SkinnedModelManager();

	/**
	 * Creates a skinned model in the manager and returns a reference to the model for modification, but the manager still owns the data.
	 * The reference parameter `outHandle` can be used after creation to reference the model or destroy it.
	 */
	SkinnedModel& createModel(Handle &outHandle);
	Handle createAssetModel(std::shared_ptr<asset::Model> asset);
	void destroyModel(Handle const& validHandle);

	void bindBuffers(Handle const& validHandle, graphics::Command *command);
	ui32 indexCount(Handle const& validHandle) const;

private:
	std::weak_ptr<GraphicsDevice> mpDevice;
	graphics::CommandPool *mpTransientCmdPool;

	std::set<Handle> mUnusedHandles;
	std::vector<SkinnedModel> mModels;
	
};

NS_END
