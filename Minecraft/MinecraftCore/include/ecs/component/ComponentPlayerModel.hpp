#pragma once

#include "ecs/component/Component.hpp"
#include "utility/DynamicHandle.hpp"

FORWARD_DEF(NS_GRAPHICS, class SkinnedModel);
FORWARD_DEF(NS_GRAPHICS, class SkinnedModelManager);
FORWARD_DEF(NS_GRAPHICS, struct EntityInstanceData);
FORWARD_DEF(NS_GRAPHICS, class EntityInstanceBuffer);
FORWARD_DEF(NS_GRAPHICS, class DescriptorSet);

NS_ECS
NS_COMPONENT

class PlayerModel : public Component
{
	DECLARE_ECS_COMPONENT_STATICS(1)

public:
	PlayerModel();
	~PlayerModel();

	PlayerModel& createModel(std::shared_ptr<graphics::SkinnedModelManager> modelManager);
	DynamicHandle<graphics::SkinnedModel> const& modelHandle() const;

	PlayerModel& createInstance(std::shared_ptr<graphics::EntityInstanceBuffer> instanceBuffer);
	DynamicHandle<graphics::EntityInstanceData> const& instanceHandle() const;

	PlayerModel& setTextureId(std::string const& textureId);
	std::string const& textureId() const;

private:
	DynamicHandle<graphics::SkinnedModel> mModelHandle;
	DynamicHandle<graphics::EntityInstanceData> mInstanceHandle;
	std::string mTextureId;

};

NS_END
NS_END
