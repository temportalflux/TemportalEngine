#include "node/NodeContext.hpp"

#include "utility/Casting.hpp"

#include <imgui-node-editor/imgui_node_editor.h>

using namespace node;
namespace IGNE = ax::NodeEditor;

NodeContext::NodeContext()
	: mpInternal(nullptr)
{
}

NodeContext::~NodeContext()
{
}

NodeContext& NodeContext::create()
{
	IGNE::Config cfg;
	this->mpInternal = IGNE::CreateEditor(&cfg);
	return *this;
}

void NodeContext::destroy()
{
	if (this->mpInternal != nullptr)
	{
		IGNE::DestroyEditor(as<IGNE::EditorContext>(this->mpInternal));
		this->mpInternal = nullptr;
	}
}

void NodeContext::activate()
{
	IGNE::SetCurrentEditor(as<IGNE::EditorContext>(this->mpInternal));
}

void NodeContext::deactivate()
{
	IGNE::SetCurrentEditor(nullptr);
}

void NodeContext::setShouldOpenContextMenu(EContextMenu type, ui32 id)
{
	this->mShouldOpenContextMenu[(uIndex)type] = id;
}

bool NodeContext::consumeShouldOpenContextMenu(EContextMenu type, ui32 expectedValue)
{
	auto bShouldOpen = this->mShouldOpenContextMenu[(uIndex)type] == expectedValue;
	this->mShouldOpenContextMenu[(uIndex)type] = std::nullopt;
	return bShouldOpen;
}

bool NodeContext::shouldShowContextMenu(EContextMenu type, ui32 id)
{
	if (this->consumeShouldOpenContextMenu(type, id)) return true;
	switch (type)
	{
		case EContextMenu::eGeneral: return IGNE::ShowBackgroundContextMenu();
		case EContextMenu::eNode:
		{
			auto eId = IGNE::NodeId(id);
			return IGNE::ShowNodeContextMenu(&eId);
		}
		case EContextMenu::ePin:
		{
			auto eId = IGNE::PinId(id);
			return IGNE::ShowPinContextMenu(&eId);
		}
		case EContextMenu::eLink:
		{
			auto eId = IGNE::LinkId(id);
			return IGNE::ShowLinkContextMenu(&eId);
		}
		default: return false;
	}
}
