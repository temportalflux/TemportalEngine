#include "ecs/component/ComponentPlayerModel.hpp"

#include "asset/TypedAssetPath.hpp"
#include "asset/ModelAsset.hpp"
#include "render/model/SkinnedModelManager.hpp"

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
}

void PlayerModel::createModel(std::shared_ptr<graphics::SkinnedModelManager> modelManager)
{
	this->mpModelManager = modelManager;
	// Create a skinned model instance of the player model
	this->mModelHandle = modelManager->createAssetModel(
		PLAYER_MODEL_PATH.load(asset::EAssetSerialization::Binary)
	);
}
