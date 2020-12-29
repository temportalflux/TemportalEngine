#pragma once

#include "TemportalEnginePCH.hpp"

#define NS_NODE namespace node {

NS_NODE

void begin(char const* titleId);
void end();

void beginNode(ui32 nodeId);
void endNode();

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

NS_END
