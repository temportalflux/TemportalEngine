#include "node/NodeCore.hpp"

#include <imgui-node-editor/imgui_node_editor.h>
#include <imgui-node-editor/examples/blueprints-example/utilities/drawing.h>
#include <imgui-node-editor/examples/blueprints-example/utilities/widgets.h>

namespace IGNE = ax::NodeEditor;

void node::begin(char const* titleId) { IGNE::Begin(titleId); }
void node::end() { IGNE::End(); }

ImVec2 node::screenToCanvas(ImVec2 const& screenPos) { return IGNE::ScreenToCanvas(screenPos); }
ImVec2 node::canvasToScreen(ImVec2 const& canvasPos) { return IGNE::CanvasToScreen(canvasPos); }

void node::setNodePosition(ui32 nodeId, math::Vector2 const& position)
{
	IGNE::SetNodePosition(IGNE::NodeId(nodeId), ImVec2(position.x(), position.y()));
}

math::Vector2 node::getNodePosition(ui32 nodeId)
{
	auto v = IGNE::GetNodePosition(IGNE::NodeId(nodeId));
	return math::Vector2{ v.x, v.y };
}

void node::beginNode(ui32 nodeId)
{
	IGNE::BeginNode(IGNE::NodeId(nodeId));
	IGNE::PushStyleVar(IGNE::StyleVar::StyleVar_NodePadding, ImVec4(8, 4, 8, 8));
	ImGui::PushID(nodeId);
}

void node::endNode()
{
	ImGui::PopID();
	IGNE::EndNode();
	IGNE::PopStyleVar();
}

void node::deleteNode(ui32 id) { IGNE::DeleteNode(IGNE::NodeId(id)); }

void node::beginPin(ui32 pinId, node::EPinType type)
{
	IGNE::BeginPin(IGNE::PinId(pinId), IGNE::PinKind(type));
}

void node::endPin()
{
	IGNE::EndPin();
}

void node::pin(
	ui32 pinId, const char* titleId,
	node::EPinType type, node::EPinIconType icon,
	math::Vector3UInt const& color,
	bool bActive, bool bEnabled
)
{
	static f32 PinIconSize = 24;

	ui32 alpha = bEnabled ? 255 : 48;
	ImVec4 innerColor = ImColor(32, 32, 32, alpha);
	ImVec4 outerColor = ImColor(
		(i32)color.x(), (i32)color.y(), (i32)color.z(), (i32)alpha
	);

	node::beginPin(pinId, type);

	IGNE::PinPivotAlignment(ImVec2(type == EPinType::eOutput ? 1.0f : 0.0f, 0.5f));
	IGNE::PinPivotSize(ImVec2(0, 0));
	ImGui::BeginGroup();
	if (type == EPinType::eOutput)
	{
		ImGui::Text(titleId);
		ImGui::SameLine();
	}
	ax::Widgets::Icon(
		ImVec2(PinIconSize, PinIconSize),
		ax::Drawing::IconType(icon),
		bActive,
		outerColor, innerColor
	);
	if (type == EPinType::eInput)
	{
		ImGui::SameLine();
		ImGui::Text(titleId);
	}
	
	ImGui::EndGroup();

	node::endPin();
}

void node::linkPins(ui32 linkId, ui32 startPinId, ui32 endPinId)
{
	IGNE::Link(linkId, startPinId, endPinId);
}
void node::deleteLink(ui32 id) { IGNE::DeleteLink(IGNE::LinkId(id)); }

void node::navigateToContent() { IGNE::NavigateToContent(); }

node::ECreateType node::beginCreate(ui32 &outStartPinId, ui32 &outEndPinId)
{
	if (IGNE::BeginCreate())
	{
		auto startPinId = IGNE::PinId(0);
		auto endPinId = IGNE::PinId(0);
		if (IGNE::QueryNewNode(&startPinId))
		{
			outStartPinId = (ui32)startPinId.Get();
			return ECreateType::eNode;
		}
		else if (IGNE::QueryNewLink(&startPinId, &endPinId))
		{
			outStartPinId = (ui32)startPinId.Get();
			outEndPinId = (ui32)endPinId.Get();
			return ECreateType::eLink;
		}
	}
	return ECreateType::eInactive;
}

bool node::acceptCreate() { return IGNE::AcceptNewItem(); }
void node::rejectCreate() { IGNE::RejectNewItem(); }
void node::endCreate() { IGNE::EndCreate(); }

bool node::beginDelete() { return IGNE::BeginDelete(); }
bool node::hasLinkToDelete(ui32 &outLinkId)
{
	auto linkId = IGNE::LinkId(0);
	if (IGNE::QueryDeletedLink(&linkId))
	{
		outLinkId = (ui32)linkId.Get();
		return true;
	}
	return false;
}
bool node::hasNodeToDelete(ui32 &outNodeId)
{
	auto nodeId = IGNE::NodeId(0);
	if (IGNE::QueryDeletedNode(&nodeId))
	{
		outNodeId = (ui32)nodeId.Get();
		return true;
	}
	return false;
}
bool node::acceptDelete() { return IGNE::AcceptDeletedItem(); }
void node::rejectDelete() { IGNE::RejectDeletedItem(); }
void node::endDelete() { IGNE::EndDelete(); }

void node::suspendGraph()
{
	auto* p = IGNE::GetCurrentEditor();
	if (p) IGNE::Suspend();
}
void node::resumeGraph()
{
	auto* p = IGNE::GetCurrentEditor();
	if (p) IGNE::Resume();
}
