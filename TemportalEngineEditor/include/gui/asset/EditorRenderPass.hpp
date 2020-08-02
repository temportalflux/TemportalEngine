#pragma once

#include "gui/asset/AssetEditor.hpp"

#include "asset/TypedAssetPath.hpp"

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

};

NS_END
