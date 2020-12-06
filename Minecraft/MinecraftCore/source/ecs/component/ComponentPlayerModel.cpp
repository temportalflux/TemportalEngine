#include "ecs/component/ComponentPlayerModel.hpp"

#include "asset/TypedAssetPath.hpp"
#include "asset/ModelAsset.hpp"
#include "render/model/SkinnedModelManager.hpp"
#include "render/EntityInstanceBuffer.hpp"

using namespace ecs;
using namespace ecs::component;

static asset::TypedAssetPath<asset::Model> PLAYER_MODEL_PATH = asset::TypedAssetPath<asset::Model>::Create(
	"assets/entity/player/PlayerModel.te-asset"
);

DEFINE_ECS_COMPONENT_STATICS(PlayerModel)

PlayerModel::PlayerModel() : Component()
{
}

PlayerModel::~PlayerModel()
{
	this->mpModelManager.lock()->destroyModel(this->mModelHandle);
	this->instanceBuffer()->destroyInstance(this->mInstanceHandle);
}

void PlayerModel::createModel(std::shared_ptr<graphics::SkinnedModelManager> modelManager)
{
	this->mpModelManager = modelManager;
	// Create a skinned model instance of the player model
	this->mModelHandle = modelManager->createAssetModel(
		PLAYER_MODEL_PATH.load(asset::EAssetSerialization::Binary)
	);
}

void PlayerModel::createInstance(std::shared_ptr<graphics::EntityInstanceBuffer> instanceBuffer)
{
	this->mpInstanceBuffer = instanceBuffer;
	this->mInstanceHandle = instanceBuffer->createInstance();
}

uIndex const& PlayerModel::modelHandle() const
{
	return this->mModelHandle;
}

std::shared_ptr<graphics::EntityInstanceBuffer> PlayerModel::instanceBuffer() const
{
	return this->mpInstanceBuffer.lock();
}

uIndex const& PlayerModel::instanceHandle() const
{
	return this->mInstanceHandle;
}
