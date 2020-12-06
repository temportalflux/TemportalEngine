#pragma once

#include "ecs/component/Component.hpp"

FORWARD_DEF(NS_GRAPHICS, class SkinnedModelManager);
FORWARD_DEF(NS_GRAPHICS, class EntityInstanceBuffer);

NS_ECS
NS_COMPONENT

class PlayerModel : public Component
{
	DECLARE_ECS_COMPONENT_STATICS(1)

public:
	PlayerModel();
	~PlayerModel();

	void createModel(std::shared_ptr<graphics::SkinnedModelManager> modelManager);
	void createInstance(std::shared_ptr<graphics::EntityInstanceBuffer> instanceBuffer);

	std::shared_ptr<graphics::EntityInstanceBuffer> instanceBuffer() const;
	uIndex const& instanceHandle() const;

private:
	std::weak_ptr<graphics::SkinnedModelManager> mpModelManager;
	uIndex mModelHandle;
	std::weak_ptr<graphics::EntityInstanceBuffer> mpInstanceBuffer;
	uIndex mInstanceHandle;

};

NS_END
NS_END
