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

	bool shouldShowContextMenu(EContextMenu type, ui32 *outId = 0);

private:
	/*EditorContext*/ void* mpInternal;

};

NS_END
