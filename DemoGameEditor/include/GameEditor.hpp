#pragma once

#include "Editor.hpp"

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
	}

	void registerAssetEditors() override
	{
		Editor::registerAssetEditors();
	}

};
