#include "asset/AssetTypeRegistry.hpp"

using namespace asset;

void TypeRegistry::registerType(AssetType type, TypeData metadata)
{
	// Assumes that the type has not been registered before
	assert(this->mAssetTypes.find(type) == this->mAssetTypes.end());
	// Add type and metadata to mappings
	this->mAssetTypes.insert(type);
	this->mAssetTypeMap.insert(std::make_pair(type, metadata));
	// Ensure that if the extension isn't already cataloged, it gets added to the set
	if (!this->isValidAssetExtension(metadata.fileExtension))
	{
		this->mAssetTypeExtensions.insert(metadata.fileExtension);
	}
}

std::set<AssetType> TypeRegistry::getAssetTypes() const
{
	return this->mAssetTypes;
}

bool TypeRegistry::isValidAssetExtension(std::string extension) const
{
	return this->mAssetTypeExtensions.find(extension) != this->mAssetTypeExtensions.end();
}

TypeData TypeRegistry::getTypeData(AssetType type) const
{
	auto iter = this->mAssetTypeMap.find(type);
	assert(iter != this->mAssetTypeMap.end());
	return iter->second;
}

std::string TypeRegistry::getTypeDisplayName(AssetType type) const
{
	return this->getTypeData(type).DisplayName;
}
