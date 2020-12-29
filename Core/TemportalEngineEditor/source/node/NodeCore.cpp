#include "node/NodeCore.hpp"

#include <imgui-node-editor/imgui_node_editor.h>
#include <imgui-node-editor/examples/blueprints-example/utilities/drawing.h>
#include <imgui-node-editor/examples/blueprints-example/utilities/widgets.h>

namespace IGNE = ax::NodeEditor;

void node::begin(char const* titleId) { IGNE::Begin(titleId); }
void node::end() { IGNE::End(); }

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

	IGNE::PinPivotAlignment(ImVec2(1.0f, 0.5f));
	IGNE::PinPivotSize(ImVec2(0, 0));
	ImGui::BeginGroup();
	ImGui::Text(titleId);
	ImGui::SameLine();
	ax::Widgets::Icon(
		ImVec2(PinIconSize, PinIconSize),
		ax::Drawing::IconType(icon),
		bActive,
		outerColor, innerColor
	);
	ImGui::EndGroup();

	node::endPin();
}
