#include "asset/ModelAsset.hpp"

#include "asset/AssetManager.hpp"
#include "cereal/list.hpp"
#include "cereal/mathVector.hpp"

using namespace asset;

DEFINE_FACTORY_ASSET_METADATA(Model)

Model::Model(std::filesystem::path filePath) : Asset(filePath)
{
}

void Model::setSourcePath(std::filesystem::path sourceFilePath)
{
	this->mSourceFilePath = sourceFilePath.string();
}

std::filesystem::path Model::getAbsoluteSourcePath() const
{
	return std::filesystem::absolute(this->getPath().parent_path() / this->mSourceFilePath);
}

void Model::setSourceBinary(std::vector<ModelVertex> const& vertices, std::vector<ui32> const& indices)
{
	this->mVertices = vertices;
	this->mIndices = indices;
}

std::vector<ModelVertex> const& Model::vertices() const
{
	return this->mVertices;
}

std::vector<ui32> const& Model::indices() const
{
	return this->mIndices;
}

void Model::onPreMoveAsset(std::filesystem::path const& prevAbsolute, std::filesystem::path const& nextAbsolute)
{
	auto srcImportPath = std::filesystem::path(this->mSourceFilePath);
	if (srcImportPath.parent_path() == prevAbsolute.parent_path())
	{
		std::filesystem::rename(srcImportPath, std::filesystem::absolute(nextAbsolute.parent_path() / srcImportPath.filename()));
	}
}

#pragma region Serialization

void Model::write(cereal::JSONOutputArchive &archive, bool bCheckDefaults) const
{
	Asset::write(archive, bCheckDefaults);
	archive(cereal::make_nvp("source", this->mSourceFilePath));
}

void Model::read(cereal::JSONInputArchive &archive, bool bCheckDefaults)
{
	Asset::read(archive, bCheckDefaults);
	archive(cereal::make_nvp("source", this->mSourceFilePath));
}

void Model::compile(cereal::PortableBinaryOutputArchive &archive, bool bCheckDefaults) const
{
	Asset::compile(archive, bCheckDefaults);
	archive(this->mVertices);
	archive(this->mIndices);
}

void Model::decompile(cereal::PortableBinaryInputArchive &archive, bool bCheckDefaults)
{
	Asset::decompile(archive, bCheckDefaults);
	archive(this->mVertices);
	archive(this->mIndices);
}

#pragma endregion
