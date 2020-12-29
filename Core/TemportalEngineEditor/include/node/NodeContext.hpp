#pragma once

#include "node/NodeCore.hpp"

NS_NODE

class NodeContext
{

public:
	NodeContext();
	NodeContext(NodeContext const& other) = delete;
	~NodeContext();

	NodeContext& create();
	void destroy();

	void activate();
	void deactivate();

private:
	/*EditorContext*/ void* mpInternal;

};

NS_END
