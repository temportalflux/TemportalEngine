#pragma once

#include "ecs/component/Component.hpp"

FORWARD_DEF(NS_GRAPHICS, class SkinnedModelManager);

NS_ECS
NS_COMPONENT

class PlayerModel : public Component
{
	DECLARE_ECS_COMPONENT_STATICS(1)

public:
	PlayerModel();
	~PlayerModel();

	void createModel(std::shared_ptr<graphics::SkinnedModelManager> modelManager);

private:
	std::weak_ptr<graphics::SkinnedModelManager> mpModelManager;
	uIndex mModelHandle;

};

NS_END
NS_END
