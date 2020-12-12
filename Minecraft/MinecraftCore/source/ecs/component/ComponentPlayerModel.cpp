#include "ecs/component/ComponentPlayerModel.hpp"

#include "Game.hpp"
#include "asset/ModelAsset.hpp"
#include "render/model/SkinnedModelManager.hpp"
#include "render/EntityInstanceBuffer.hpp"

using namespace ecs;
using namespace ecs::component;

DEFINE_ECS_COMPONENT_STATICS(PlayerModel)

PlayerModel::PlayerModel() : Component()
{
	auto game = game::Game::Get();
	this->mModelHandle = game->modelManager()->createHandle();
	this->mInstanceHandle = game->entityInstances()->createHandle();
}

PlayerModel::~PlayerModel()
{
	this->mModelHandle.destroy();
	this->mInstanceHandle.destroy();
}

PlayerModel& PlayerModel::setModel(asset::TypedAssetPath<asset::Model> const& path)
{
	this->mModelHandle.owner<graphics::SkinnedModelManager>()
		->setModel(this->mModelHandle, path.load(asset::EAssetSerialization::Binary));
	return *this;
}

PlayerModel& PlayerModel::setModel(render::SimpleModel const& simple)
{
	this->mModelHandle.owner<graphics::SkinnedModelManager>()
		->setModel(this->mModelHandle, simple.vertices, simple.indices);
	return *this;
}

DynamicHandle<graphics::SkinnedModel> const& PlayerModel::modelHandle() const { return this->mModelHandle; }

DynamicHandle<graphics::EntityInstanceData> const& PlayerModel::instanceHandle() const { return this->mInstanceHandle; }

PlayerModel& PlayerModel::setTextureId(std::string const& textureId)
{
	this->mTextureId = textureId;
	return *this;
}

std::string const& PlayerModel::textureId() const { return this->mTextureId; }
