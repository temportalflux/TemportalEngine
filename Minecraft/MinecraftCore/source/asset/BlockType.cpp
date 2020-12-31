#include "asset/BlockType.hpp"

#include "asset/AssetManager.hpp"

using namespace asset;

DEFINE_PROPERTY_CONTAINER(BlockType)
DEFINE_FACTORY_ASSET_METADATA(BlockType)

DEFINE_ASSET_CONSTRUCTORS(BlockType)
	: mbIsTranslucent(false)
{
}

game::BlockId const& BlockType::uniqueId() const
{
	return this->mId;
}

CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, BlockType::write, cereal::JSONOutputArchive, BlockType::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, BlockType::read, cereal::JSONInputArchive, BlockType::deserialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, BlockType::compile, cereal::PortableBinaryOutputArchive, BlockType::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, BlockType::decompile, cereal::PortableBinaryInputArchive, BlockType::deserialize);
