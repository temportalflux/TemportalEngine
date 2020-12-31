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
	this->mpRootNode = std::reinterpret_pointer_cast<RootNode>(this->createNode(ENodeType::eRoot));	

	std::map<uIndex, std::shared_ptr<AttachmentNode>> attachmentToNode;
	std::map<uIndex, std::shared_ptr<PhaseNode>> phaseToNode;

	auto const& attachments = asset->getAttachments();
	for (uIndex idx = 0; idx < attachments.size(); ++idx)
	{
		auto node = std::reinterpret_pointer_cast<AttachmentNode>(this->createNode(ENodeType::eAttachment));
		node->assetData = attachments[idx];
		attachmentToNode.insert(std::make_pair(idx, node));
	}

	auto const& phases = asset->getPhases();
	for (uIndex idxPhase = 0; idxPhase < phases.size(); ++idxPhase)
	{
		auto node = std::reinterpret_pointer_cast<PhaseNode>(this->createNode(ENodeType::ePhase));
		node->assetData = phases[idxPhase];
		phaseToNode.insert(std::make_pair(idxPhase, node));

		auto& phase = phases[idxPhase].data;
		node->colorAttachments.resize(phase.colorAttachments.size());
		for (uIndex idxReference = 0; idxReference < phase.colorAttachments.size(); ++idxReference)
		{
			node->colorAttachments[idxReference] = this->createAttachmentReference(node->nodeId);

			auto attachmentRef = phase.colorAttachments[idxReference];
			if (attachmentRef)
			{
				node->colorAttachments[idxReference].layout = attachmentRef->layout;
				this->addLink(
					attachmentToNode[attachmentRef->attachment]->pinId,
					node->colorAttachments[idxReference].pinId
				);
			}
		}
		if (phase.depthAttachment)
		{
			node->depthAttachment.layout = phase.depthAttachment->layout;
			this->addLink(
				attachmentToNode[phase.depthAttachment->attachment]->pinId,
				node->depthAttachment.pinId
			);
		}
	}

	for (auto const& dep : asset->getPhaseDependencies())
	{
		auto node = std::reinterpret_pointer_cast<DependencyNode>(this->createNode(ENodeType::eDependency));
		node->assetData = dep;

		if (dep.bPrevPhaseIsRoot)
		{
			this->addLink(this->mpRootNode->pinId, node->pinIdPrevPhase);
		}
		else if (dep.data.prev.phase)
		{
			this->addLink(
				phaseToNode[*dep.data.prev.phase]->pinIdSupportedDependencies,
				node->pinIdPrevPhase
			);
		}
		if (dep.data.next.phase)
		{
			this->addLink(
				node->pinIdNextPhase,
				phaseToNode[*dep.data.next.phase]->pinIdRequiredDependencies
			);
		}
	}
}

EditorRenderPass::PhaseNode::AttachmentReference EditorRenderPass::createAttachmentReference(ui32 nodeId)
{
	auto pinId = this->createPin(nodeId, ENodeType::eAttachment, true);
	this->mPins[pinId].deleteCallback = std::bind(&EditorRenderPass::deleteAttachmentReference, this, nodeId, pinId);
	return { pinId, graphics::EImageLayout::eUndefined };
}

void EditorRenderPass::deleteAttachmentReference(ui32 nodeId, ui32 pinId)
{
	auto phaseNode = std::reinterpret_pointer_cast<PhaseNode>(this->mNodes[nodeId]);
	auto iter = phaseNode->colorAttachments.begin();
	while (iter != phaseNode->colorAttachments.end())
	{
		if (iter->pinId == pinId) break;
		++iter;
	}
	if (iter != phaseNode->colorAttachments.end())
	{
		phaseNode->colorAttachments.erase(iter);
		this->deletePin(pinId);
	}
}

void EditorRenderPass::renderContent()
{
	AssetEditor::renderContent();

	this->mNodeCtx.activate();
	node::begin(this->titleId().c_str());

	node::setNodePosition(0, math::Vector2::ZERO);
	this->rootNode(this->mpRootNode->nodeId, this->mpRootNode->pinId);

	for (auto& [nodeId, node]: this->mNodes)
	{
		switch (node->type)
		{
			case ENodeType::eAttachment:
				this->renderAttachmentNode(std::reinterpret_pointer_cast<AttachmentNode>(node));
				break;
			case ENodeType::ePhase:
				this->renderPhaseNode(std::reinterpret_pointer_cast<PhaseNode>(node));
				break;
			case ENodeType::eDependency:
				this->renderDependencyNode(std::reinterpret_pointer_cast<DependencyNode>(node));
				break;
			case ENodeType::eRoot:
			case ENodeType::eInvalid:
				break;
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
	{
		auto root = std::make_shared<RootNode>();
		root->pinId = this->createPin(nodeId, ENodeType::ePhase, false);
		node = root;
		break;
	}
	case ENodeType::eAttachment:
	{
		auto phase = std::make_shared<AttachmentNode>();
		phase->pinId = this->createPin(nodeId, ENodeType::eAttachment, false);
		node = phase;
		break;
	}
	case ENodeType::ePhase:
	{
		auto phase = std::make_shared<PhaseNode>();
		phase->pinIdRequiredDependencies = this->createPin(nodeId, ENodeType::eDependency, false);
		phase->pinIdSupportedDependencies = this->createPin(nodeId, ENodeType::ePhase, false);
		phase->colorAttachments = {};
		phase->depthAttachment = {
			this->createPin(nodeId, ENodeType::eAttachment, true),
			graphics::EImageLayout::eUndefined
		};
		node = phase;
		break;
	}
	case ENodeType::eDependency:
	{
		auto dependency = std::make_shared<DependencyNode>();
		dependency->pinIdPrevPhase = this->createPin(nodeId, ENodeType::ePhase, true);
		dependency->pinIdNextPhase = this->createPin(nodeId, ENodeType::eDependency, true);
		node = dependency;
		break;
	}
	}
	node->nodeId = nodeId;
	node->type = type;
	this->mNodes.insert(std::make_pair(nodeId, node));
	return node;
}

ui32 EditorRenderPass::createPin(ui32 nodeId, ENodeType pinType, bool bSingleLinkOnly)
{
	auto id = this->nextId();
	this->mPins.insert(std::make_pair(id, Pin { id, nodeId, pinType, bSingleLinkOnly, {} }));
	return id;
}

void EditorRenderPass::addLink(ui32 startPinId, ui32 endPinId)
{
	auto linkId = this->nextId();
	this->mLinks.insert(std::make_pair(linkId, Link{ linkId, startPinId, endPinId }));
	
	auto& startPin = this->mPins.at(startPinId);
	if (startPin.bSingleLinkOnly && startPin.linkIds.size() > 0)
	{
		node::deleteLink(*startPin.linkIds.begin());
	}
	startPin.linkIds.insert(linkId);

	auto& endPin = this->mPins.at(endPinId);
	if (endPin.bSingleLinkOnly && endPin.linkIds.size() > 0)
	{
		node::deleteLink(*endPin.linkIds.begin());
	}
	endPin.linkIds.insert(linkId);
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
		comboPopup = EditorRenderPass::ComboPopup{ popupId, &value.data(), true, {} };
		for (const auto& option : value.all())
		{
			comboPopup->options.push_back(EditorRenderPass::ComboPopup::Option{
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

template <typename TEnum>
bool renderEnum(
	char const* id, utility::EnumWrapper<TEnum> &value,
	std::string const& popupId,
	std::optional<EditorRenderPass::ComboPopup> &comboPopup, std::optional<std::string> &popupToOpenFlag
)
{
	ImGui::PushID(id);

	if (ImGui::ArrowButton("###edit", ImGuiDir_Down))
	{
		popupToOpenFlag = popupId;
		comboPopup = EditorRenderPass::ComboPopup{ popupId, &value.data(), false, {} };
		for (const auto& option : utility::EnumWrapper<TEnum>::ALL)
		{
			comboPopup->options.push_back(EditorRenderPass::ComboPopup::Option{
				ui64(option), utility::EnumWrapper<TEnum>(option).to_display_string()
			});
		}
	}
	ImGui::SameLine();
	ImGui::Text(value.to_display_string().c_str());

	ImGui::PopID();
	return false;
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
		pin.linkIds.size() > 0,
		this->isPinValidTarget(type, pin.pinId)
	);
}

void EditorRenderPass::rootNode(ui32 nodeId, ui32 phasePinId)
{
	auto asset = this->get<asset::RenderPass>();
	node::beginNode(nodeId);

	ImGui::Text("Root");
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(100, 0));
	ImGui::SameLine();
	this->renderPin(this->mPins[phasePinId], "Dependencies", node::EPinType::eOutput);

	ImGui::Spacing();

	ImGui::PushItemWidth(180);
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

void EditorRenderPass::renderAttachmentNode(std::shared_ptr<AttachmentNode> node)
{
	if (this->mbIsFirstFrame)
	{
		node::setNodePosition(node->nodeId, node->assetData.nodePosition);
	}

	node::beginNode(node->nodeId);
	
	ImGui::Text("Attachment");
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(200, 0));
	ImGui::SameLine();
	this->renderPin(this->mPins[node->pinId], "", node::EPinType::eOutput);

	ImGui::Spacing();

	auto& attachment = node->assetData.data;

	ImGui::Text("Format");
	ImGui::SameLine();
	if (renderEnum("###format", attachment.formatType, "format", this->mComboPopup, this->mComboPopupToOpen))
		this->markAssetDirty(1);

	ImGui::Text("Samples");
	ImGui::SameLine();
	if (renderEnum("###samples", attachment.samples, "samples", this->mComboPopup, this->mComboPopupToOpen))
		this->markAssetDirty(1);

	{
		ImGui::Text("Layout");
		ImGui::Indent();
		ImGui::Text("Initial");
		ImGui::SameLine();
		if (renderEnum("###initial", attachment.initialLayout, "initialLayout", this->mComboPopup, this->mComboPopupToOpen))
			this->markAssetDirty(1);
		ImGui::Text("Final");
		ImGui::SameLine();
		if (renderEnum("###final", attachment.finalLayout, "finalLayout", this->mComboPopup, this->mComboPopupToOpen))
			this->markAssetDirty(1);
		ImGui::Unindent();
	}

	{
		ImGui::PushID("general");
		ImGui::Text("General Operations");
		ImGui::Indent();
		ImGui::Text("Load");
		ImGui::SameLine();
		if (renderEnum("###load", attachment.generalLoadOp, "generalLoad", this->mComboPopup, this->mComboPopupToOpen))
			this->markAssetDirty(1);
		ImGui::Text("Store");
		ImGui::SameLine();
		if (renderEnum("###store", attachment.generalStoreOp, "generalStore", this->mComboPopup, this->mComboPopupToOpen))
			this->markAssetDirty(1);
		ImGui::Unindent();
		ImGui::PopID();
	}


	{
		ImGui::PushID("stencil");
		ImGui::Text("Stencil Operations");
		ImGui::Indent();
		ImGui::Text("Load");
		ImGui::SameLine();
		if (renderEnum("###load", attachment.stencilLoadOp, "stencilLoad", this->mComboPopup, this->mComboPopupToOpen))
			this->markAssetDirty(1);
		ImGui::Text("Store");
		ImGui::SameLine();
		if (renderEnum("###store", attachment.stencilStoreOp, "stencilStore", this->mComboPopup, this->mComboPopupToOpen))
			this->markAssetDirty(1);
		ImGui::Unindent();
		ImGui::PopID();
	}

	node::endNode();

	auto pos = node::getNodePosition(node->nodeId);
	if (pos != node->assetData.nodePosition)
	{
		node->assetData.nodePosition = pos;
		this->markAssetDirty(1);
	}
}

void EditorRenderPass::renderPhaseNode(std::shared_ptr<PhaseNode> node)
{
	if (this->mbIsFirstFrame)
	{
		node::setNodePosition(node->nodeId, node->assetData.nodePosition);
	}

	node::beginNode(node->nodeId);
	ImGui::Text("Phase");

	this->renderPin(this->mPins[node->pinIdRequiredDependencies], "Requirements", node::EPinType::eInput);
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(150, 0));
	ImGui::SameLine();
	this->renderPin(this->mPins[node->pinIdSupportedDependencies], "Supports", node::EPinType::eOutput);

	ImGui::Spacing();

	ImGui::Text("Color Attachments");
	ImGui::SameLine();
	if (ImGui::Button("+"))
	{
		node->colorAttachments.push_back(this->createAttachmentReference(node->nodeId));
	}
	for (uIndex i = 0; i < node->colorAttachments.size(); ++i)
	{
		this->renderAttachmentReference(std::to_string(i).c_str(), node->colorAttachments[i]);
	}

	this->renderAttachmentReference("Depth Attachment", node->depthAttachment);

	node::endNode();

	auto pos = node::getNodePosition(node->nodeId);
	if (pos != node->assetData.nodePosition)
	{
		node->assetData.nodePosition = pos;
		this->markAssetDirty(1);
	}
}

void EditorRenderPass::renderAttachmentReference(char const* id, PhaseNode::AttachmentReference &value)
{
	ImGui::PushID(id);
	this->renderPin(this->mPins[value.pinId], id, node::EPinType::eInput);
	ImGui::Indent();
	ImGui::Text("Input Layout");
	ImGui::SameLine();
	if (this->renderImageLayout("###layout", "attachRefLayout", value.layout)) this->markAssetDirty(1);
	ImGui::Unindent();
	ImGui::PopID();
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
	ImGui::PushItemWidth(180);
	{
		this->renderPin(this->mPins[node->pinIdPrevPhase], "Previous", node::EPinType::eInput);
		auto& prev = node->assetData.data.prev;
		ImGui::PushID("prev");
		if (this->renderStageMask("Stages", utility::formatStr("%i prev stage", node->nodeId), prev.stageMask)) this->markAssetDirty(1);
		if (this->renderAccessMask("Access", utility::formatStr("%i prev access", node->nodeId), prev.accessMask)) this->markAssetDirty(1);
		ImGui::PopID();

		ImGui::Dummy(ImVec2(200, 0));
		ImGui::SameLine();
		this->renderPin(this->mPins[node->pinIdNextPhase], "Next", node::EPinType::eOutput);
		auto& next = node->assetData.data.next;
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

bool EditorRenderPass::renderStageMask(char const* id, std::string const& popupId, graphics::PipelineStageMask &value)
{
	return renderFlags(id, value, popupId, this->mComboPopup, this->mComboPopupToOpen);
}

bool EditorRenderPass::renderAccessMask(char const* id, std::string const& popupId, graphics::AccessMask &value)
{
	return renderFlags(id, value, popupId, this->mComboPopup, this->mComboPopupToOpen);
}

bool EditorRenderPass::renderImageLayout(char const* id, std::string const& popupId, graphics::ImageLayout &value)
{
	return renderEnum(id, value, popupId, this->mComboPopup, this->mComboPopupToOpen);
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
			this->markAssetDirty(1);
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
					auto startIter = this->mPins.find(linkIter->second.startPinId);
					if (startIter != this->mPins.end())
						startIter->second.linkIds.erase(startIter->second.linkIds.find(linkId));

					auto endIter = this->mPins.find(linkIter->second.endPinId);
					if (endIter != this->mPins.end())
						endIter->second.linkIds.erase(endIter->second.linkIds.find(linkId));

					this->mLinks.erase(linkIter);
					this->markAssetDirty(1);
				}
			}
		}

		ui32 nodeId;
		while (node::hasNodeToDelete(nodeId))
		{
			// cannot delete the root node
			if (nodeId == this->mpRootNode->nodeId)
			{
				node::rejectDelete();
				continue;
			}
			if (node::acceptDelete())
			{
				auto nodeIter = this->mNodes.find(nodeId);
				if (nodeIter != this->mNodes.end())
				{
					this->deleteNodePins(nodeIter->second);
					this->mNodes.erase(nodeIter);
					this->markAssetDirty(1);
				}
			}
		}
	}
	node::endDelete();
}

void EditorRenderPass::deleteNodePins(std::shared_ptr<Node> node)
{
	switch (node->type)
	{
		case ENodeType::eAttachment:
		{
			auto attachment = std::reinterpret_pointer_cast<AttachmentNode>(node);
			this->deletePin(attachment->pinId);
			break;
		}
		case ENodeType::ePhase:
		{
			auto phase = std::reinterpret_pointer_cast<PhaseNode>(node);
			this->deletePin(phase->pinIdRequiredDependencies);
			this->deletePin(phase->pinIdSupportedDependencies);	
			for (auto const& attachment : phase->colorAttachments) this->deletePin(attachment.pinId);
			this->deletePin(phase->depthAttachment.pinId);
			break;
		}
		case ENodeType::eDependency:
		{
			auto dep = std::reinterpret_pointer_cast<DependencyNode>(node);
			this->deletePin(dep->pinIdPrevPhase);
			this->deletePin(dep->pinIdNextPhase);
			break;
		}
		default: break;
	}
}

void EditorRenderPass::deletePin(ui32 pinId)
{
	auto iter = this->mPins.find(pinId);
	for (auto const& linkId : iter->second.linkIds) node::deleteLink(linkId);
	this->mPins.erase(iter);
}

void EditorRenderPass::renderContextMenus()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	this->renderContextNewNode();
	this->renderContextNode();
	this->renderContextPin();
	this->renderContextLink();
	this->renderContextComboPopup();
	ImGui::PopStyleVar();
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
		if ((!startPinDataType) && ImGui::MenuItem("Attachment"))
		{
			auto node = std::reinterpret_pointer_cast<AttachmentNode>(this->createNode(ENodeType::eAttachment));
			node::setNodePosition(node->nodeId, { nodePos.x, nodePos.y });
			this->markAssetDirty(1);
		}
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

void EditorRenderPass::renderContextNode()
{
	if (this->mNodeCtx.shouldShowContextMenu(node::NodeContext::EContextMenu::eNode, &this->mContextMenuId))
	{
		ImGui::OpenPopup("nodeCtx");
	}
	if (ImGui::BeginPopup("nodeCtx"))
	{
		auto& node = this->mNodes[this->mContextMenuId];
		switch (node->type)
		{
			case ENodeType::eRoot: ImGui::Text("Root Node"); break;
			case ENodeType::ePhase: ImGui::Text("Phase Node"); break;
			case ENodeType::eDependency: ImGui::Text("Dependency Node"); break;
			case ENodeType::eAttachment: ImGui::Text("Attachment Node"); break;
			default: break;
		}
		if (node->type != ENodeType::eRoot && ImGui::MenuItem("Delete Node")) node::deleteNode(this->mContextMenuId);
		ImGui::EndPopup();
	}
}

void EditorRenderPass::renderContextPin()
{
	if (this->mNodeCtx.shouldShowContextMenu(node::NodeContext::EContextMenu::ePin, &this->mContextMenuId))
	{
		ImGui::OpenPopup("pinCtx");
	}
	if (ImGui::BeginPopup("pinCtx"))
	{
		auto const& pin = this->mPins[this->mContextMenuId];
		if (pin.linkIds.size() > 0 && ImGui::MenuItem("Unlink All"))
		{
			for (auto const& linkId : pin.linkIds) node::deleteLink(linkId);
		}
		if (pin.deleteCallback && ImGui::MenuItem("Delete Pin")) pin.deleteCallback();
		ImGui::EndPopup();
	}
}

void EditorRenderPass::renderContextLink()
{
	if (this->mNodeCtx.shouldShowContextMenu(node::NodeContext::EContextMenu::eLink, &this->mContextMenuId))
	{
		ImGui::OpenPopup("linkCtx");
	}
	if (ImGui::BeginPopup("linkCtx"))
	{
		if (ImGui::MenuItem("Sever")) { node::deleteLink(this->mContextMenuId); }
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
			bool bIsSelected = this->mComboPopup->bIsMask ? (data & option.flag) == option.flag : data == option.flag;
			ImGui::PushID((ui32)option.flag);
			if (ImGui::Selectable(option.displayName.c_str(), bIsSelected))
			{
				if (this->mComboPopup->bIsMask)
				{
					if (!bIsSelected) data |= option.flag;
					else data ^= option.flag;
					this->markAssetDirty(1);
				}
				else if (!bIsSelected)
				{
					data = option.flag;
					this->markAssetDirty(1);
				}
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
			if (this->mCreateLink->endPinId) return false;
			return this->canLinkPins(*this->mCreateLink->startPinId, pinId);
		case node::EPinType::eOutput:
			if (this->mCreateLink->startPinId) return false;
			return this->canLinkPins(pinId, *this->mCreateLink->endPinId);
		default: return true;
	}
}

bool EditorRenderPass::canLinkPins(ui32 startPinId, ui32 endPinId) const
{
	auto const& startPin = this->mPins.at(startPinId);
	auto const& endPin = this->mPins.at(endPinId);
	return startPin.pinType == endPin.pinType;
}

void EditorRenderPass::saveAsset()
{
	auto asset = this->get<asset::RenderPass>();

	auto& attachments = asset->ref_Attachments();
	attachments.clear();
	auto& phases = asset->ref_Phases();
	phases.clear();
	auto& dependencies = asset->ref_PhaseDependencies();
	dependencies.clear();

	std::map<ui32, uIndex> attachmentNodeIdToIdx;
	std::map<ui32, uIndex> phaseNodeIdToIdx;

	for (auto& [nodeId, node] : this->mNodes)
	{
		if (node->type != ENodeType::eAttachment) continue;
		auto attachmentNode = std::reinterpret_pointer_cast<AttachmentNode>(node);
		attachmentNodeIdToIdx.insert(std::make_pair(node->nodeId, attachments.size()));
		attachments.push_back(attachmentNode->assetData);
	}

	auto createAttachementReference = [&](ui32 linkId, graphics::EImageLayout layout)
	{
		auto attachmentPin = this->mPins[this->mLinks[linkId].startPinId];
		auto idxAttachment = attachmentNodeIdToIdx[attachmentPin.nodeId];
		return graphics::RenderPassAttachmentReference { idxAttachment, layout };
	};
	for (auto& [nodeId, node] : this->mNodes)
	{
		if (node->type != ENodeType::ePhase) continue;
		auto phaseNode = std::reinterpret_pointer_cast<PhaseNode>(node);
		phaseNodeIdToIdx.insert(std::make_pair(node->nodeId, phases.size()));

		auto& phase = phaseNode->assetData.data;
		phase.colorAttachments.clear();
		for (auto const& attachment : phaseNode->colorAttachments)
		{
			auto& pin = this->mPins[attachment.pinId];
			phase.colorAttachments.push_back(
				pin.linkIds.size() == 1
				? std::make_optional(createAttachementReference(
					*pin.linkIds.begin(), attachment.layout
				))
				: std::nullopt
			);
		}
		auto& depthPin = this->mPins[phaseNode->depthAttachment.pinId];
		phase.depthAttachment =
			depthPin.linkIds.size() == 1
			? std::make_optional(createAttachementReference(
				*depthPin.linkIds.begin(), phaseNode->depthAttachment.layout
			))
			: std::nullopt;

		phases.push_back(phaseNode->assetData);
	}
	for (auto&[nodeId, node] : this->mNodes)
	{
		if (node->type != ENodeType::eDependency) continue;
		auto dependencyNode = std::reinterpret_pointer_cast<DependencyNode>(node);
		auto const& prevPin = this->mPins[dependencyNode->pinIdPrevPhase];
		assert(prevPin.linkIds.size() <= 1);
		if (prevPin.linkIds.size() > 0)
		{
			auto prevPhasePinId = this->mLinks[*prevPin.linkIds.begin()].startPinId;
			auto prevPhaseNodeId = this->mPins[prevPhasePinId].nodeId;
			auto& prevPhaseIdx = dependencyNode->assetData.data.prev.phase;
			if (prevPhaseNodeId == this->mpRootNode->nodeId)
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
		for (auto const& linkId : nextPin.linkIds)
		{
			auto nextPhasePinId = this->mLinks[linkId].endPinId;
			auto nextPhaseNodeId = this->mPins[nextPhasePinId].nodeId;
			auto nextPhaseIdx = phaseNodeIdToIdx[nextPhaseNodeId];
			dependencyNode->assetData.data.next.phase = nextPhaseIdx;
			// this is an intentional copy so that the data for each next phase is copied properly
			dependencies.push_back(dependencyNode->assetData);
		}
	}

	AssetEditor::saveAsset();
}
