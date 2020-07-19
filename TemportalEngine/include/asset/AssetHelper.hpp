#pragma once

// The extension that all assets use by default unless otherwise specified (special cases include Project and Editor Settings)
#define DEFAULT_ASSET_EXTENSION ".te-asset"
#define ASSET_CATEGORY_GENERAL "General"
#define ASSET_CATEGORY_GRAPHICS "Graphics"
// Relevant methods for determining the asset type key for a given Asset subclass or instance
#define DEFINE_ASSET_TYPE(TYPE_STR) static constexpr char const* StaticType() { return TYPE_STR; } virtual AssetType getAssetType() const override { return StaticType(); }
// Static Constexpr for the display name of a given asset type
#define DEFINE_ASSET_DISPLAYNAME(DisplayName) static constexpr char const* TypeDisplayName() { return DisplayName; }
// Static Constexpr for the extension of a given asset type
#define DEFINE_ASSET_EXTENSION(EXTENSION) static constexpr char const* TypeExtension() { return EXTENSION; }
#define DEFINE_ASSET_CATEGORY(CATEGORY) static constexpr char const* TypeCategory() { return CATEGORY; }
// Defines & Declares all the relevant static functions needed during asset registration (see ASSET_TYPE_METADATA)
#define DEFINE_ASSET_STATICS(TYPE_STR, DISPLAY_NAME, EXTENSION, CATEGORY) \
	DEFINE_ASSET_TYPE(TYPE_STR) \
	DEFINE_ASSET_DISPLAYNAME(DISPLAY_NAME) \
	DEFINE_ASSET_EXTENSION(EXTENSION) \
	DEFINE_ASSET_CATEGORY(CATEGORY)

// Macros for defining the function to create an asset for a type
#define DECLARE_FACTORY_ASSET_NEW() static asset::AssetPtrStrong createNewAsset(std::filesystem::path filePath);
#define DEFINE_FACTORY_ASSET_NEW(ClassType) asset::AssetPtrStrong ClassType::createNewAsset(std::filesystem::path filePath) { return asset::AssetManager::makeAsset<ClassType>(filePath); }

// Macros for defining the function to create an empty asset for a type
#define DECLARE_FACTORY_ASSET_EMPTY() static asset::AssetPtrStrong createEmptyAsset();
#define DEFINE_FACTORY_ASSET_EMPTY(ClassType) asset::AssetPtrStrong ClassType::createEmptyAsset() { return asset::AssetManager::makeAsset<ClassType>(); }

// Macros for defining the function to delete an asset for a type
#define DECLARE_FACTORY_ASSET_DELETE() static void onAssetDeleted(std::filesystem::path filePath);
#define DEFINE_FACTORY_ASSET_DELETE(ClassType) void ClassType::onAssetDeleted(std::filesystem::path filePath) {}

// Macros for defining the static function callbacks for asset factory management
#define DECLARE_FACTORY_ASSET_METADATA() DECLARE_FACTORY_ASSET_NEW() DECLARE_FACTORY_ASSET_EMPTY() DECLARE_FACTORY_ASSET_DELETE()
#define DEFINE_FACTORY_ASSET_METADATA(ClassType) DEFINE_FACTORY_ASSET_NEW(ClassType) DEFINE_FACTORY_ASSET_EMPTY(ClassType) DEFINE_FACTORY_ASSET_DELETE(ClassType)

// Used to register an asset type's metadata with the AssetManager
#define ASSET_TYPE_METADATA(ClassType) { ClassType::TypeDisplayName(), ClassType::TypeExtension(), &ClassType::createNewAsset, &ClassType::createEmptyAsset, &ClassType::onAssetDeleted }

#define CREATE_NEWASSET_CONSTRUCTOR(ClassType) ClassType(std::filesystem::path filePath) : Asset(filePath)

#define DECLARE_SERIALIZATION_METHOD(method, archiveType, post) void method(archiveType &archive) post;
#define CREATE_DEFAULT_SERIALIZATION_DEFINITION(post, classAndMethod, archiveType, rootMethod) void classAndMethod(archiveType &archive) post { rootMethod(archive); }
#define NOOP_SERIALIZATION_METHOD(method, archiveType, post) void method(archiveType &archive) post { assert(false); }
