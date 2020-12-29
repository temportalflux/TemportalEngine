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
