#pragma once

#include "gui/asset/AssetEditor.hpp"

#include "asset/TypedAssetPath.hpp"
#include "graphics/Area.hpp"
#include "graphics/RenderPassMeta.hpp"
#include "node/NodeContext.hpp"

FORWARD_DEF(NS_ASSET, class Pipeline);
FORWARD_DEF(NS_ASSET, class RenderPass);

NS_GUI

// Editor for `asset::RenderPass`
class EditorRenderPass : public AssetEditor
{

public:
	static std::shared_ptr<AssetEditor> create(std::shared_ptr<memory::MemoryChunk> mem);

	EditorRenderPass();
	~EditorRenderPass();
	void setAsset(asset::AssetPtrStrong asset) override;

protected:
	void renderContent() override;
	void saveAsset() override;

private:
	std::vector<graphics::RPPhase> mPhases;
	std::vector<graphics::RPDependency> mPhaseDependencies;

	std::vector<asset::AssetPath> mAllPipelinePaths;
	std::vector<std::string> mPhaseNames;

	std::function<bool(uIndex const& idx, graphics::RPPhase &phase)> mfRenderPhaseName;
	std::function<bool(uIndex const& idx, graphics::RPDependency &dep)> mfRenderDependencyKey;
	std::function<bool(uIndex const& idx, graphics::RPDependency &dep)> mfRenderDependency;
	std::function<bool(uIndex &idx)> mfRenderDependencyItemName;

	std::vector<std::string> makePhaseNames();
	bool renderPhaseName(uIndex const &idx, graphics::RPPhase &phase);
	bool renderPhaseDependencyKey(uIndex const &idx, graphics::RPDependency &dependency);
	bool renderPhaseDependency(uIndex const &idx, graphics::RPDependency &dependency);
	bool renderPhaseDependencyItem(graphics::RPDependency::Item &item);
	bool renderPhaseDependencyItemPhaseName(uIndex &phaseIdx);

	node::NodeContext mNodeCtx;

	void rootNode(ui32 nodeId, ui32 phasePinId);

};

NS_END
