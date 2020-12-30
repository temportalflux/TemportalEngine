#include "gui/asset/EditorRenderPass.hpp"

#include "asset/AssetManager.hpp"
#include "memory/MemoryChunk.hpp"
#include "utility/StringUtils.hpp"

#include "property/Property.hpp"

#include <imgui.h>
#include <imgui-node-editor/imgui_node_editor.h>

using namespace gui;

math::Vector3UInt COLOR_PIN_PHASE = { 255, 48, 48 };
math::Vector3UInt COLOR_PIN_DEPENDENCY = { 68, 201, 156 };
math::Vector3UInt COLOR_PIN_ATTACHMENT = { 23, 79, 232 };

// Macros for the bits each field of the project asset correspond to in the dirty flags

std::shared_ptr<AssetEditor> EditorRenderPass::create(std::shared_ptr<memory::MemoryChunk> mem)
{
	return mem->make_shared<EditorRenderPass>();
}

EditorRenderPass::EditorRenderPass() : mbIsFirstFrame(true)
{
	this->mNodeCtx.create();
}

EditorRenderPass::~EditorRenderPass()
{
	this->mNodeCtx.destroy();
}

ui32 EditorRenderPass::nextId()
{
	if (this->mUnusedIds.empty()) return this->mIdCount++;
	else
	{
		ui32 i = *this->mUnusedIds.begin();
		this->mUnusedIds.erase(this->mUnusedIds.begin());
		return i;
	}
}

void EditorRenderPass::setAsset(asset::AssetPtrStrong assetGeneric)
{
	AssetEditor::setAsset(assetGeneric);

	auto asset = this->get<asset::RenderPass>();
	
	// Root node is 0, root node out is 1
	auto rootNode = this->createNode(ENodeType::eRoot);
	this->createPin(rootNode->nodeId, ENodeType::ePhase);

	std::map<uIndex, std::shared_ptr<PhaseNode>> phaseToNode;

	auto const& phases = asset->getPhaseNodes();
	for (uIndex idxPhase = 0; idxPhase < phases.size(); ++idxPhase)
	{
		auto node = std::reinterpret_pointer_cast<PhaseNode>(this->createNode(ENodeType::ePhase));
		node->assetData = phases[idxPhase];
		phaseToNode.insert(std::make_pair(idxPhase, node));
	}

	auto const& phaseDependencies = asset->getPhaseDependencyNodes();
	for (auto const& dep : asset->getPhaseDependencyNodes())
	{
		auto node = std::reinterpret_pointer_cast<DependencyNode>(this->createNode(ENodeType::eDependency));
		node->assetData = dep;

		if (dep.bPrevPhaseIsRoot)
		{
			this->addLink(1, node->pinIdPrevPhase);
		}
		else if (dep.data.dependee.phaseIndex)
		{
			this->addLink(
				phaseToNode[*dep.data.dependee.phaseIndex]->pinIdSupportedDependencies,
				node->pinIdPrevPhase
			);
		}
		if (dep.data.depender.phaseIndex)
		{
			this->addLink(
				node->pinIdNextPhase,
				phaseToNode[*dep.data.depender.phaseIndex]->pinIdRequiredDependencies
			);
		}
	}
}

void EditorRenderPass::renderContent()
{
	AssetEditor::renderContent();

	this->mNodeCtx.activate();
	node::begin(this->titleId().c_str());

	node::setNodePosition(0, math::Vector2::ZERO);
	this->rootNode(0, 1);

	for (auto& [nodeId, node]: this->mNodes)
	{
		switch (node->type)
		{
			case ENodeType::ePhase:
				this->renderPhaseNode(std::reinterpret_pointer_cast<PhaseNode>(node));
				break;
			case ENodeType::eDependency:
				this->renderDependencyNode(std::reinterpret_pointer_cast<DependencyNode>(node));
				break;
			default: break;
		}
	}

	for (auto const& [linkId, link] : this->mLinks)
	{
		node::linkPins(linkId, link.startPinId, link.endPinId);
	}

	this->pollCreateBuffer();
	this->pollDeleteBuffer();

	node::suspendGraph();
	this->renderContextMenus();
	node::resumeGraph();

	if (this->mbIsFirstFrame)
	{
		node::navigateToContent();
		this->mbIsFirstFrame = false;
	}

	node::end();
	this->mNodeCtx.deactivate();
}

std::shared_ptr<EditorRenderPass::Node> EditorRenderPass::createNode(ENodeType type)
{
	auto nodeId = this->nextId();
	std::shared_ptr<Node> node;
	switch (type)
	{
	case ENodeType::eRoot:
		node = std::make_shared<Node>();
		break;
	case ENodeType::ePhase:
	{
		auto phase = std::make_shared<PhaseNode>();
		phase->pinIdRequiredDependencies = this->createPin(nodeId, ENodeType::eDependency);
		phase->pinIdSupportedDependencies = this->createPin(nodeId, ENodeType::ePhase);
		phase->colorAttachmentPinIds = std::vector<ui32>();
		phase->pinIdDepthAttachment = this->createPin(nodeId, ENodeType::eAttachment);
		node = phase;
		break;
	}
	case ENodeType::eDependency:
	{
		auto dependency = std::make_shared<DependencyNode>();
		dependency->pinIdPrevPhase = this->createPin(nodeId, ENodeType::ePhase);
		dependency->pinIdNextPhase = this->createPin(nodeId, ENodeType::eDependency);
		node = dependency;
		break;
	}
	}
	node->nodeId = nodeId;
	node->type = type;
	this->mNodes.insert(std::make_pair(nodeId, node));
	return node;
}

ui32 EditorRenderPass::createPin(ui32 nodeId, ENodeType pinType)
{
	auto id = this->nextId();
	this->mPins.insert(std::make_pair(id, Pin { id, nodeId, pinType }));
	return id;
}

void EditorRenderPass::addLink(ui32 startPinId, ui32 endPinId)
{
	auto linkId = this->nextId();
	this->mLinks.insert(std::make_pair(linkId, Link{ linkId, startPinId, endPinId }));
	this->mPins.at(startPinId).linkId = linkId;
	this->mPins.at(endPinId).linkId = linkId;
}

math::Vector3UInt EditorRenderPass::getColorForPinType(ENodeType type)
{
	switch (type)
	{
		case ENodeType::ePhase: return COLOR_PIN_PHASE;
		case ENodeType::eDependency: return COLOR_PIN_DEPENDENCY;
		case ENodeType::eAttachment: return COLOR_PIN_ATTACHMENT;
		default: return {};
	}
}

void EditorRenderPass::renderPin(Pin const& pin, char const* titleId, node::EPinType type)
{
	node::pin(
		pin.pinId, titleId, type, node::EPinIconType::eCircle,
		this->getColorForPinType(pin.pinType),
		pin.linkId.has_value(),
		this->isPinValidTarget(type, pin.pinId)
	);
}

void EditorRenderPass::rootNode(ui32 nodeId, ui32 phasePinId)
{
	auto asset = this->get<asset::RenderPass>();
	node::beginNode(nodeId);

	ImGui::Text("Root");
	ImGui::SameLine();
	this->renderPin(this->mPins[phasePinId], "Dependencies", node::EPinType::eOutput);

	ImGui::Spacing();

	ImGui::PushItemWidth(150);
	{
		ImGui::Text("Area");
		ImGui::Indent();
		{
			auto& value = asset->REF_PROP(RenderArea);
			auto const& defaultValue = asset->DEFAULT_PROP(RenderArea);
			if (properties::renderProperty("Offset", value.offset, defaultValue.offset)) this->markAssetDirty(1);
			if (properties::renderProperty("Size", value.size, defaultValue.size)) this->markAssetDirty(1);
		}
		ImGui::Unindent();

		{
			auto& value = asset->REF_PROP(ClearColor);
			bool bToggledOn = value.has_value();
			if (ImGui::Checkbox("Clear Color", &bToggledOn) && bToggledOn != value.has_value())
			{
				value = bToggledOn ? std::make_optional(math::Color()) : std::nullopt;
				this->markAssetDirty(1);
			}
			if (value.has_value())
			{
				ImGui::Indent();
				if (properties::renderProperty("###value", *value, math::Color())) this->markAssetDirty(1);
				ImGui::Unindent();
			}
		}

		{
			auto& value = asset->REF_PROP(ClearDepthStencil);
			bool bToggledOn = value.has_value();
			if (ImGui::Checkbox("Clear Depth/Stencil", &bToggledOn) && bToggledOn != value.has_value())
			{
				value = bToggledOn ? std::make_optional(std::pair<f32, ui32>()) : std::nullopt;
				this->markAssetDirty(1);
			}
			if (value.has_value())
			{
				ImGui::Indent();
				if (properties::renderProperty("Depth", value->first, 0.0f)) this->markAssetDirty(1);
				if (properties::renderProperty("Stencil", value->second, (ui32)0)) this->markAssetDirty(1);
				ImGui::Unindent();
			}
		}

	}
	ImGui::PopItemWidth();

	node::endNode();
}

void EditorRenderPass::renderPhaseNode(std::shared_ptr<PhaseNode> node)
{
	if (this->mbIsFirstFrame)
	{
		node::setNodePosition(node->nodeId, node->assetData.nodePosition);
	}

	node::beginNode(node->nodeId);
	ImGui::Text("Phase");
	ImGui::Spacing();

	this->renderPin(this->mPins[node->pinIdRequiredDependencies], "Requirements", node::EPinType::eInput);
	ImGui::SameLine();
	this->renderPin(this->mPins[node->pinIdSupportedDependencies], "Supports", node::EPinType::eOutput);

	ImGui::Spacing();

	ImGui::Text("Color Attachments");

	this->renderPin(this->mPins[node->pinIdDepthAttachment], "Depth Attachment", node::EPinType::eInput);

	node::endNode();

	auto pos = node::getNodePosition(node->nodeId);
	if (pos != node->assetData.nodePosition)
	{
		node->assetData.nodePosition = pos;
		this->markAssetDirty(1);
	}
}

void EditorRenderPass::renderDependencyNode(std::shared_ptr<DependencyNode> node)
{
	if (this->mbIsFirstFrame)
	{
		node::setNodePosition(node->nodeId, node->assetData.nodePosition);
	}

	node::beginNode(node->nodeId);
	ImGui::Text("Dependency");
	ImGui::Spacing();
	ImGui::PushItemWidth(150);
	{
		this->renderPin(this->mPins[node->pinIdPrevPhase], "Previous", node::EPinType::eInput);
		auto& prev = node->assetData.data.dependee;
		ImGui::PushID("prev");
		if (this->renderStageMask("Stages", utility::formatStr("%i prev stage", node->nodeId), prev.stageMask)) this->markAssetDirty(1);
		if (this->renderAccessMask("Access", utility::formatStr("%i prev access", node->nodeId), prev.accessMask)) this->markAssetDirty(1);
		ImGui::PopID();

		this->renderPin(this->mPins[node->pinIdNextPhase], "Next", node::EPinType::eOutput);
		auto& next = node->assetData.data.depender;
		ImGui::PushID("next");
		if (this->renderStageMask("Stages", utility::formatStr("%i next stage", node->nodeId), next.stageMask)) this->markAssetDirty(1);
		if (this->renderAccessMask("Access", utility::formatStr("%i next access", node->nodeId), next.accessMask)) this->markAssetDirty(1);
		ImGui::PopID();
	}
	ImGui::PopItemWidth();
	node::endNode();

	auto pos = node::getNodePosition(node->nodeId);
	if (pos != node->assetData.nodePosition)
	{
		node->assetData.nodePosition = pos;
		this->markAssetDirty(1);
	}
}

template <typename TEnum>
bool renderFlags(
	char const* id, utility::Flags<TEnum> &value,
	std::string const& popupId,
	std::optional<EditorRenderPass::ComboPopup> &comboPopup, std::optional<std::string> &popupToOpenFlag
)
{
	ImGui::PushID(id);

	ImGui::Text(id);
	ImGui::SameLine();
	if (ImGui::Button("Edit"))
	{
		popupToOpenFlag = popupId;
		comboPopup = EditorRenderPass::ComboPopup { popupId, &value.data(), {} };
		for (const auto& option : value.all())
		{
			comboPopup->options.push_back(EditorRenderPass::ComboPopup::Option {
				ui64(option), utility::EnumWrapper<TEnum>(option).to_display_string()
			});
		}
	}

	ImGui::Indent();
	{
		for (const auto& option : value.all())
		{
			if ((value.data() & ui64(option)) == ui64(option))
			{
				ImGui::Text(utility::EnumWrapper<TEnum>(option).to_display_string().c_str());
			}
		}
	}
	ImGui::Unindent();

	ImGui::PopID();
	return false;
}

bool EditorRenderPass::renderStageMask(char const* id, std::string const& popupId, graphics::PipelineStageMask &value)
{
	return renderFlags(id, value, popupId, this->mComboPopup, this->mComboPopupToOpen);
}

bool EditorRenderPass::renderAccessMask(char const* id, std::string const& popupId, graphics::AccessMask &value)
{
	return renderFlags(id, value, popupId, this->mComboPopup, this->mComboPopupToOpen);
}

void EditorRenderPass::pollCreateBuffer()
{
	// When the user begins to create a node or link
	ui32 startPinId, endPinId;
	switch (node::beginCreate(startPinId, endPinId))
	{
	case node::ECreateType::eNode:
		if (node::acceptCreate())
		{
			this->mCreateNodePinId = startPinId;
			this->mbOpenCreateNodeMenu = true;
		}
		break;
	case node::ECreateType::eLink:
	{
		this->mCreateLink = {
			startPinId != 0 ? std::make_optional(startPinId) : std::nullopt,
			endPinId != 0 ? std::make_optional(endPinId) : std::nullopt,
		};
		bool bValidPins = this->mCreateLink->startPinId && this->mCreateLink->endPinId && startPinId != endPinId;
		if (bValidPins && this->canLinkPins(startPinId, endPinId) && node::acceptCreate())
		{
			this->addLink(startPinId, endPinId);
			this->mCreateLink = std::nullopt;
		}
		break;
	}
	case node::ECreateType::eInactive:
		this->mCreateLink = std::nullopt;
		break;
	}
	node::endCreate();
}

void EditorRenderPass::pollDeleteBuffer()
{
	if (node::beginDelete())
	{
		ui32 linkId;
		while (node::hasLinkToDelete(linkId))
		{
			if (node::acceptDelete())
			{
				auto linkIter = this->mLinks.find(linkId);
				if (linkIter != this->mLinks.end())
				{
					this->mPins[linkIter->second.startPinId].linkId = std::nullopt;
					this->mPins[linkIter->second.endPinId].linkId = std::nullopt;
					this->mLinks.erase(linkIter);
				}
			}
		}

		ui32 nodeId;
		while (node::hasNodeToDelete(nodeId))
		{
			if (node::acceptDelete())
			{
				auto nodeIter = this->mNodes.find(nodeId);
				if (nodeIter != this->mNodes.end()) this->mNodes.erase(nodeIter);
				// TODO: delete pins
			}
		}
	}
	node::endDelete();
}

void EditorRenderPass::renderContextMenus()
{
	this->renderContextNewNode();
	this->renderContextComboPopup();
}

void EditorRenderPass::renderContextNewNode()
{
	if (this->mbOpenCreateNodeMenu)
	{
		ImGui::OpenPopup("newNode");
		this->mbOpenCreateNodeMenu = false;
	}
	else if (this->mNodeCtx.shouldShowContextMenu(node::NodeContext::EContextMenu::eGeneral))
	{
		this->mCreateNodePinId = std::nullopt;
		ImGui::OpenPopup("newNode");
	}

	if (ImGui::BeginPopup("newNode"))
	{
		auto nodePos = node::screenToCanvas(ImGui::GetMousePosOnOpeningCurrentPopup());
		std::optional<ENodeType> startPinDataType = std::nullopt;
		if (this->mCreateNodePinId) startPinDataType = this->mPins.at(*this->mCreateNodePinId).pinType;
		if ((!startPinDataType || startPinDataType == ENodeType::ePhase) && ImGui::MenuItem("Phase"))
		{
			auto node = std::reinterpret_pointer_cast<PhaseNode>(this->createNode(ENodeType::ePhase));
			if (this->mCreateNodePinId)
			{
				this->addLink(*this->mCreateNodePinId, node->pinIdRequiredDependencies);
			}
			node::setNodePosition(node->nodeId, { nodePos.x, nodePos.y });
			this->markAssetDirty(1);
		}
		if ((!startPinDataType || startPinDataType == ENodeType::eDependency) && ImGui::MenuItem("Dependency"))
		{
			auto node = std::reinterpret_pointer_cast<DependencyNode>(this->createNode(ENodeType::eDependency));
			if (this->mCreateNodePinId)
			{
				this->addLink(*this->mCreateNodePinId, node->pinIdPrevPhase);
			}
			node::setNodePosition(node->nodeId, { nodePos.x, nodePos.y });
			this->markAssetDirty(1);
		}
		ImGui::EndPopup();
	}
}

void EditorRenderPass::renderContextComboPopup()
{
	if (!this->mComboPopup) return;
	if (this->mComboPopupToOpen)
	{
		ImGui::OpenPopup(this->mComboPopupToOpen->c_str());
		this->mComboPopupToOpen = std::nullopt;
	}
	if (ImGui::BeginPopup(this->mComboPopup->popupId.c_str()))
	{
		ui64& data = *this->mComboPopup->comboData;
		for (auto const& option : this->mComboPopup->options)
		{
			bool bIsSelected = (data & option.flag) == option.flag;
			ImGui::PushID((ui32)option.flag);
			if (ImGui::Selectable(option.displayName.c_str(), bIsSelected))
			{
				if (!bIsSelected) data |= option.flag;
				else data ^= option.flag;
				this->markAssetDirty(1);
			}
			ImGui::PopID();
		}
		ImGui::EndPopup();
	}
	else
	{
		this->mComboPopup = std::nullopt;
	}
}

bool EditorRenderPass::isPinValidTarget(node::EPinType type, ui32 pinId) const
{
	if (!this->mCreateLink) return true;
	switch (type)
	{
		case node::EPinType::eInput:
			if (!this->mCreateLink->startPinId) return false;
			return this->canLinkPins(*this->mCreateLink->startPinId, pinId);
		case node::EPinType::eOutput:
			if (!this->mCreateLink->endPinId) return false;
			return this->canLinkPins(pinId, *this->mCreateLink->endPinId);
		default: return true;
	}
}

bool EditorRenderPass::canLinkPins(ui32 startPinId, ui32 endPinId) const
{
	auto const& startPin = this->mPins.at(startPinId);
	//bool bStartIsPhase = startPin.nodeType == ENodeType::eRoot || startPin.nodeType == ENodeType::ePhase;
	auto const& endPin = this->mPins.at(endPinId);
	//bool bEndIsPhase = endPin.nodeType == ENodeType::eRoot || endPin.nodeType == ENodeType::ePhase;
	return true;
}

void EditorRenderPass::saveAsset()
{
	auto asset = this->get<asset::RenderPass>();
	
	auto& phases = asset->ref_PhaseNodes();
	phases.clear();
	auto& dependencies = asset->ref_PhaseDependencyNodes();
	dependencies.clear();

	std::map<ui32, uIndex> phaseNodeIdToIdx;
	for (auto& [nodeId, node] : this->mNodes)
	{
		if (node->type != ENodeType::ePhase) continue;
		auto phaseNode = std::reinterpret_pointer_cast<PhaseNode>(node);
		phaseNodeIdToIdx.insert(std::make_pair(node->nodeId, phases.size()));
		phases.push_back(phaseNode->assetData);
	}
	for (auto&[nodeId, node] : this->mNodes)
	{
		if (node->type != ENodeType::eDependency) continue;
		auto dependencyNode = std::reinterpret_pointer_cast<DependencyNode>(node);
		auto const& prevPin = this->mPins[dependencyNode->pinIdPrevPhase];
		if (prevPin.linkId)
		{
			auto prevPhasePinId = this->mLinks[*prevPin.linkId].startPinId;
			auto prevPhaseNodeId = this->mPins[prevPhasePinId].nodeId;
			auto& prevPhaseIdx = dependencyNode->assetData.data.dependee.phaseIndex;
			if (prevPhaseNodeId == 0)
			{
				prevPhaseIdx = std::nullopt;
				dependencyNode->assetData.bPrevPhaseIsRoot = true;
			}
			else
			{
				prevPhaseIdx = phaseNodeIdToIdx[prevPhaseNodeId];
			}
		}
		auto const& nextPin = this->mPins[dependencyNode->pinIdNextPhase];
		if (nextPin.linkId)
		{
			auto nextPhasePinId = this->mLinks[*nextPin.linkId].endPinId;
			auto nextPhaseNodeId = this->mPins[nextPhasePinId].nodeId;
			auto prevPhaseIdx = phaseNodeIdToIdx[nextPhaseNodeId];
			dependencyNode->assetData.data.depender.phaseIndex = prevPhaseIdx;
			dependencies.push_back(dependencyNode->assetData);
		}
	}

	AssetEditor::saveAsset();
}
