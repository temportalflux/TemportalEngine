#pragma once

#include "asset/TypedAssetPath.hpp"
#include "ecs/component/Component.hpp"
#include "render/ModelSimple.hpp"
#include "utility/DynamicHandle.hpp"

FORWARD_DEF(NS_GRAPHICS, class SkinnedModel);
FORWARD_DEF(NS_GRAPHICS, class SkinnedModelManager);
FORWARD_DEF(NS_GRAPHICS, struct EntityInstanceData);
FORWARD_DEF(NS_GRAPHICS, class EntityInstanceBuffer);
FORWARD_DEF(NS_GRAPHICS, class DescriptorSet);
FORWARD_DEF(NS_ASSET, class Model);

NS_ECS
NS_COMPONENT

class RenderMesh : public Component
{
	DECLARE_ECS_COMPONENT_STATICS(16)

public:
	RenderMesh();
	~RenderMesh();

	RenderMesh& setModel(asset::TypedAssetPath<asset::Model> const& path);
	RenderMesh& setModel(render::SimpleModel const& simple);
	DynamicHandle<graphics::SkinnedModel> const& modelHandle() const;

	DynamicHandle<graphics::EntityInstanceData> const& instanceHandle() const;

	RenderMesh& setTextureId(std::string const& textureId);
	std::string const& textureId() const;

private:
	DynamicHandle<graphics::SkinnedModel> mModelHandle;
	DynamicHandle<graphics::EntityInstanceData> mInstanceHandle;
	std::string mTextureId;

};

NS_END
NS_END
