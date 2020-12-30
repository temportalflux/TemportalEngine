#pragma once

#include "gui/asset/AssetEditor.hpp"

#include "asset/TypedAssetPath.hpp"
#include "asset/RenderPassAsset.hpp"
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
	bool mbIsFirstFrame;

	ui32 mIdCount;
	std::set<ui32> mUnusedIds;

	node::NodeContext mNodeCtx;
	bool mbOpenCreateNodeMenu;
	std::optional<ui32> mCreateNodePinId;

	struct CreateLink
	{
		std::optional<ui32> startPinId;
		std::optional<ui32> endPinId;
	};
	std::optional<CreateLink> mCreateLink;

	enum class ENodeType : i8
	{
		eInvalid = -1,
		eRoot = 0,
		ePhase,
		eDependency,
		eAttachment,
	};
	struct Node
	{
		ui32 nodeId;
		ENodeType type;
	};
	struct PhaseNode : public Node
	{
		ui32 pinIdRequiredDependencies;
		ui32 pinIdSupportedDependencies;
		std::vector<ui32> colorAttachmentPinIds;
		ui32 pinIdDepthAttachment;
		asset::RenderPass::NodePhase assetData;
	};
	struct DependencyNode : public Node
	{
		ui32 pinIdPrevPhase;
		ui32 pinIdNextPhase;
		asset::RenderPass::NodePhaseDependency assetData;
	};
	std::map<ui32, std::shared_ptr<Node>> mNodes;
	struct Pin
	{
		ui32 pinId;
		ui32 nodeId;
		ENodeType pinType;
		bool bSingleLinkOnly;
		std::set<ui32> linkIds;
	};
	std::map<ui32, Pin> mPins;
	struct Link
	{
		ui32 linkId;
		ui32 startPinId;
		ui32 endPinId;
	};
	std::map<ui32, Link> mLinks;
	
	ui32 nextId();
	
	std::shared_ptr<Node> createNode(ENodeType type);
	ui32 createPin(ui32 nodeId, ENodeType nodeType, bool bSingleLinkOnly);
	void addLink(ui32 startPinId, ui32 endPinId);

	math::Vector3UInt getColorForPinType(ENodeType type);

	void renderPin(Pin const& pin, char const* titleId, node::EPinType type);
	void rootNode(ui32 nodeId, ui32 phasePinId);
	void renderPhaseNode(std::shared_ptr<PhaseNode> node);
	void renderDependencyNode(std::shared_ptr<DependencyNode> node);
	
	void pollCreateBuffer();
	void pollDeleteBuffer();
	void deleteNodePins(std::shared_ptr<Node> node);

	void renderContextMenus();
	void renderContextNewNode();

	ui32 mContextMenuId;
	void renderContextNode();
	void renderContextPin();
	void renderContextLink();

	bool isPinValidTarget(node::EPinType type, ui32 pinId) const;
	bool canLinkPins(ui32 startPinId, ui32 endPinId) const;

	bool renderStageMask(char const* id, std::string const& popupId, graphics::PipelineStageMask &value);
	bool renderAccessMask(char const* id, std::string const& popupId, graphics::AccessMask &value);

public:
	struct ComboPopup
	{
		std::string popupId;
		ui64 *comboData;
		struct Option
		{
			ui64 flag;
			std::string displayName;
		};
		std::vector<Option> options;
	};
private:
	std::optional<ComboPopup> mComboPopup;
	std::optional<std::string> mComboPopupToOpen;
	void renderContextComboPopup();

};

NS_END
