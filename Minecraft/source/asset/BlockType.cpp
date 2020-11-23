#include "asset/BlockType.hpp"

#include "asset/AssetManager.hpp"

using namespace asset;

DEFINE_FACTORY_ASSET_METADATA(BlockType)

BlockType::BlockType(std::filesystem::path filePath) : Asset(filePath)
{
}

std::vector<AssetPath const*> BlockType::getAssetRefs() const
{
	return {
		&this->mTextures.sampler.path(),
		&this->mTextures.right.path(),
		&this->mTextures.left.path(),
		&this->mTextures.front.path(),
		&this->mTextures.back.path(),
		&this->mTextures.up.path(),
		&this->mTextures.down.path()
	};
}

std::vector<AssetPath*> BlockType::getAssetRefs()
{
	return {
		&this->mTextures.sampler.path(),
		&this->mTextures.right.path(),
		&this->mTextures.left.path(),
		&this->mTextures.front.path(),
		&this->mTextures.back.path(),
		&this->mTextures.up.path(),
		&this->mTextures.down.path()
	};
}

game::BlockId const& BlockType::uniqueId() const
{
	return this->mId;
}

CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, BlockType::write, cereal::JSONOutputArchive, BlockType::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, BlockType::read, cereal::JSONInputArchive, BlockType::deserialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, BlockType::compile, cereal::PortableBinaryOutputArchive, BlockType::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, BlockType::decompile, cereal::PortableBinaryInputArchive, BlockType::deserialize);
