#pragma once

#include "gui/asset/AssetEditor.hpp"

#include "asset/PipelineAsset.hpp"
#include "graphics/BlendMode.hpp"
#include "graphics/types.hpp"
#include "graphics/Area.hpp"

NS_GUI

// Editor for `asset::Pipeline`
class EditorPipeline : public AssetEditor
{

public:
	static std::shared_ptr<AssetEditor> create(std::shared_ptr<memory::MemoryChunk> mem);

	void setAsset(asset::AssetPtrStrong asset) override;

protected:
	void renderContent() override;
	void saveAsset() override;

private:
	graphics::Viewport mViewport;
	graphics::Area mScissor;
	graphics::FrontFace::Enum mFrontFace;
	std::unordered_set<graphics::ColorComponent::Enum> mBlendWriteMask;
	std::string mBlendWriteMaskPreviewStr;
	std::optional<graphics::BlendMode::Operation> mBlendOperation;
	std::vector<asset::Pipeline::Descriptor> mDescriptors;

};

NS_END
