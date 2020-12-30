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

bool NodeContext::shouldShowContextMenu(EContextMenu type, ui32 *outId)
{
	switch (type)
	{
		case EContextMenu::eGeneral: return IGNE::ShowBackgroundContextMenu();
		case EContextMenu::eNode:
		{
			auto eId = IGNE::NodeId(0);
			if (IGNE::ShowNodeContextMenu(&eId))
			{
				*outId = (ui32)eId.Get();
				return true;
			}
			return false;
		}
		case EContextMenu::ePin:
		{
			auto eId = IGNE::PinId(0);
			if (IGNE::ShowPinContextMenu(&eId))
			{
				*outId = (ui32)eId.Get();
				return true;
			}
			return false;
		}
		case EContextMenu::eLink:
		{
			auto eId = IGNE::LinkId(0);
			if (IGNE::ShowLinkContextMenu(&eId))
			{
				*outId = (ui32)eId.Get();
				return true;
			}
			return false;
		}
		default: return false;
	}
}
