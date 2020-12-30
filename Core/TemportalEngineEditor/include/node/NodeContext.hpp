#pragma once

#include "node/NodeCore.hpp"

NS_NODE

class NodeContext
{

public:
	enum class EContextMenu : ui8
	{
		eGeneral,
		eNode,
		ePin,
		eLink,

		COUNT
	};

	NodeContext();
	NodeContext(NodeContext const& other) = delete;
	~NodeContext();

	NodeContext& create();
	void destroy();

	void activate();
	void deactivate();

	void setShouldOpenContextMenu(EContextMenu type, ui32 id = 0);
	bool shouldShowContextMenu(EContextMenu type, ui32 id = 0);

private:
	/*EditorContext*/ void* mpInternal;
	std::optional<ui32> mShouldOpenContextMenu[uSize(EContextMenu::COUNT)];

	bool consumeShouldOpenContextMenu(EContextMenu type, ui32 expectedValue);

};

NS_END
