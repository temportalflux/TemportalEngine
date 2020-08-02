#pragma once

#include "gui/asset/AssetEditor.hpp"

#include "asset/TypedAssetPath.hpp"
#include "graphics/Area.hpp"

FORWARD_DEF(NS_ASSET, class Pipeline);
FORWARD_DEF(NS_ASSET, class RenderPass);

NS_GUI

// Editor for `asset::RenderPass`
class EditorRenderPass : public AssetEditor
{

public:
	static std::shared_ptr<AssetEditor> create(std::shared_ptr<memory::MemoryChunk> mem);

	void setAsset(asset::AssetPtrStrong asset) override;

protected:
	void renderContent() override;
	void saveAsset() override;

private:
	std::optional<math::Vector4> mClearColor;
	std::optional<std::pair<f32, ui32>> mClearDepthStencil;
	graphics::Area mRenderArea;
	std::vector<asset::TypedAssetPath<asset::Pipeline>> mPipelines;

	std::vector<asset::AssetPath> mAllPipelinePaths;

};

NS_END
