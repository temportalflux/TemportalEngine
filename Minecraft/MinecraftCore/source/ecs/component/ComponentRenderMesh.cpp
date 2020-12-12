#include "ecs/component/ComponentRenderMesh.hpp"

#include "Game.hpp"
#include "asset/ModelAsset.hpp"
#include "render/model/SkinnedModelManager.hpp"
#include "render/EntityInstanceBuffer.hpp"

using namespace ecs;
using namespace ecs::component;

DEFINE_ECS_COMPONENT_STATICS(RenderMesh)

RenderMesh::RenderMesh() : Component(), mbShouldRender(true)
{
	auto game = game::Game::Get();
	this->mModelHandle = game->modelManager()->createHandle();
	this->mInstanceHandle = game->entityInstances()->createHandle();
}

RenderMesh::~RenderMesh()
{
	this->mModelHandle.destroy();
	this->mInstanceHandle.destroy();
}

RenderMesh& RenderMesh::setVisible(bool bVisible)
{
	this->mbShouldRender = bVisible;
	return *this;
}

bool RenderMesh::shouldRender() const { return this->mbShouldRender; }

RenderMesh& RenderMesh::setModel(asset::TypedAssetPath<asset::Model> const& path)
{
	this->mModelHandle.owner<graphics::SkinnedModelManager>()
		->setModel(this->mModelHandle, path.load(asset::EAssetSerialization::Binary));
	return *this;
}

RenderMesh& RenderMesh::setModel(render::SimpleModel const& simple)
{
	this->mModelHandle.owner<graphics::SkinnedModelManager>()
		->setModel(this->mModelHandle, simple.vertices, simple.indices);
	return *this;
}

DynamicHandle<graphics::SkinnedModel> const& RenderMesh::modelHandle() const { return this->mModelHandle; }

DynamicHandle<graphics::EntityInstanceData> const& RenderMesh::instanceHandle() const { return this->mInstanceHandle; }

RenderMesh& RenderMesh::setTextureId(std::string const& textureId)
{
	this->mTextureId = textureId;
	return *this;
}

std::string const& RenderMesh::textureId() const { return this->mTextureId; }
