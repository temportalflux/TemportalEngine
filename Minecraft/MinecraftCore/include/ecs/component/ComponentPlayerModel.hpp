#pragma once

#include "ecs/component/Component.hpp"
#include "utility/DynamicHandle.hpp"

FORWARD_DEF(NS_GRAPHICS, class SkinnedModel);
FORWARD_DEF(NS_GRAPHICS, class SkinnedModelManager);
FORWARD_DEF(NS_GRAPHICS, struct EntityInstanceData);
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

	DynamicHandle<graphics::SkinnedModel> const& modelHandle() const;
	DynamicHandle<graphics::EntityInstanceData> const& instanceHandle() const;

private:
	DynamicHandle<graphics::SkinnedModel> mModelHandle;
	DynamicHandle<graphics::EntityInstanceData> mInstanceHandle;

};

NS_END
NS_END
