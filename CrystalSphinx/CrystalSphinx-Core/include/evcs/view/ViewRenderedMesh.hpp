#pragma once

#include "CoreInclude.hpp"

#include "evcs/view/ECView.hpp"

NS_EVCS
NS_VIEW

class RenderedMesh : public View
{
	DECLARE_ECS_VIEW_STATICS()
public:
	RenderedMesh();
};

NS_END
NS_END
