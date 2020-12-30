#pragma once

#include "TemportalEnginePCH.hpp"

#include <imgui.h>

#define NS_NODE namespace node {
NS_NODE

void begin(char const* titleId);
void end();

ImVec2 screenToCanvas(ImVec2 const& screenPos);
ImVec2 canvasToScreen(ImVec2 const& canvasPos);

void setNodePosition(ui32 nodeId, math::Vector2 const& position);
math::Vector2 getNodePosition(ui32 nodeId);
void beginNode(ui32 nodeId);
void endNode();
void deleteNode(ui32 id);

enum class EPinType
{
	eInput,
	eOutput,
};
enum class EPinIconType : ui32
{
	eFlow,
	eCircle,
	eSquare,
	eGrid,
	eRoundSquare,
	eDiamond,
};
void beginPin(ui32 pinId, EPinType type);
void endPin();
void pin(
	ui32 pinId, const char* titleId,
	EPinType type, EPinIconType icon,
	math::Vector3UInt const& color,
	bool bActive, bool bEnabled
);

void linkPins(ui32 linkId, ui32 startPinId, ui32 endPinId);
void deleteLink(ui32 id);

void navigateToContent();

enum class ECreateType
{
	eInactive,
	eNode,
	eLink,
};
ECreateType beginCreate(ui32 &outStartPinId, ui32 &outEndPinId);
bool acceptCreate();
void rejectCreate();
void endCreate();

bool beginDelete();
bool hasLinkToDelete(ui32 &outLinkId);
bool hasNodeToDelete(ui32 &outNodeId);
bool acceptDelete();
void rejectDelete();
void endDelete();

void suspendGraph();
void resumeGraph();

NS_END
