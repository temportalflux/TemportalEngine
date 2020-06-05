#pragma once

#define DEFINE_ASSET_TYPE(TYPE_STR) virtual AssetType getAssetType() const override { return TYPE_STR; }
#define CREATE_ASSETTYPE_METADATA(ClassType, Name, Extension, DeleteMethod) { Name, Extension, &ClassType::createNewAsset, &ClassType::createEmptyAsset, DeleteMethod }

#define DECLARE_NEWASSET_FACTORY() static asset::AssetPtrStrong createNewAsset(std::filesystem::path filePath);
#define DEFINE_NEWASSET_FACTORY(ClassType) asset::AssetPtrStrong ClassType::createNewAsset(std::filesystem::path filePath) { return asset::AssetManager::makeAsset<ClassType>(filePath); }

#define DECLARE_EMPTYASSET_FACTORY() static asset::AssetPtrStrong createEmptyAsset();
#define DEFINE_EMPTYASSET_FACTORY(ClassType) asset::AssetPtrStrong ClassType::createEmptyAsset() { return asset::AssetManager::makeAsset<ClassType>(); }

#define CREATE_NEWASSET_CONSTRUCTOR(ClassType) ClassType(std::filesystem::path filePath) : Asset(filePath)

#define DECLARE_SERIALIZATION_METHOD(method, archiveType, post) void method(archiveType &archive) post;
#define CREATE_DEFAULT_SERIALIZATION_DEFINITION(post, classAndMethod, archiveType, rootMethod) void classAndMethod(archiveType &archive) post { rootMethod(archive); }
#define NOOP_SERIALIZATION_METHOD(method, archiveType, post) void method(archiveType &archive) post { assert(false); }
