#pragma once

#include "ecs/types.h"

#include "ITickable.hpp"

NS_ECS
FORWARD_DEF(NS_VIEW, class View);

NS_SYSTEM

class System : public ITickable
{

public:
	System(ViewTypeId const& viewTypeId);

	ViewTypeId const& viewId() const;
	virtual void tick(f32 deltaTime) override;

protected:
	
	virtual void update(f32 deltaTime, view::View* view) = 0;

	template <typename TView>
	TView* asView(view::View* view)
	{
		return reinterpret_cast<TView*>(view);
	}

private:
	ViewTypeId mViewTypeId;

};

NS_END
NS_END
