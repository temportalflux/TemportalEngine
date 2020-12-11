#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/Buffer.hpp"
#include "render/ModelVertex.hpp"
#include "utility/DynamicHandle.hpp"

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

	void bindBuffers(graphics::Command *command) const;
	ui32 indexCount() const;

private:
	std::vector<ModelVertex> mVertices;
	std::vector<ui32> mIndices;

	graphics::Buffer mVertexBuffer;
	graphics::Buffer mIndexBuffer;

};

class SkinnedModelManager : public IDynamicHandleOwner<SkinnedModel>
{
public:
	SkinnedModelManager(
		std::weak_ptr<GraphicsDevice> device,
		graphics::CommandPool* transientPool
	);
	~SkinnedModelManager();

	void setModel(DynamicHandle<SkinnedModel> const& handle, std::shared_ptr<asset::Model> asset);
	void setModel(DynamicHandle<SkinnedModel> const& handle, std::vector<ModelVertex> const& vertices, std::vector<ui32> const& indices);

public:
	DynamicHandle<SkinnedModel> createHandle() override;
	SkinnedModel* get(uIndex const& idx) override;
	void destroyHandle(uIndex const& idx) override;

private:
	std::weak_ptr<GraphicsDevice> mpDevice;
	graphics::CommandPool *mpTransientCmdPool;

	std::set<uIndex> mUnusedModelIndices;
	std::vector<SkinnedModel> mModels;

};

NS_END
