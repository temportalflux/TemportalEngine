#include "asset/BlockType.hpp"

#include "asset/AssetManager.hpp"

using namespace asset;

DEFINE_FACTORY_ASSET_METADATA(BlockType)

BlockType::BlockType(std::filesystem::path filePath) : Asset(filePath)
{
}

std::string const BlockType::uniqueId() const
{
	return this->mId.moduleName + ":" + this->mId.name;
}

CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, BlockType::write, cereal::JSONOutputArchive, BlockType::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, BlockType::read, cereal::JSONInputArchive, BlockType::deserialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, BlockType::compile, cereal::PortableBinaryOutputArchive, BlockType::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, BlockType::decompile, cereal::PortableBinaryInputArchive, BlockType::deserialize);
