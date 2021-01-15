#pragma once

#include "Editor.hpp"

#include "asset/AssetManager.hpp"
#include "asset/BlockType.hpp"
#include "build/asset/BuildAsset.hpp"

/**
 * A subclass of `Editor` specific for DemoGame
 */
class GameEditor : public Editor
{

public:
	GameEditor(int argc, char *argv[]) : Editor(argc, argv) {}

private:
	void registerAssetTypes(std::shared_ptr<asset::AssetManager> assetManager) override
	{
		Editor::registerAssetTypes(assetManager);
		assetManager->registerType<asset::BlockType>();
	}

	void registerAssetBuilders() override
	{
		Editor::registerAssetBuilders();
		this->registerAssetBuilder(asset::BlockType::StaticType(), &build::BuildAsset::create);
	}

	void registerAssetEditors() override
	{
		Editor::registerAssetEditors();
	}

};
