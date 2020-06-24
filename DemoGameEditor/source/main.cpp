#include "Editor.hpp"
#include "Engine.hpp"
#include "asset/AssetManager.hpp"

std::shared_ptr<ui8> GamePtr;

void bindEngineHooks(std::shared_ptr<engine::Engine> engine);
void registerAssetTypes(std::shared_ptr<asset::AssetManager> assetManager);
void registerAssetEditors(Editor* editor);

int main(int argc, char *argv[])
{
	// TODO: All of this behavior should be wrapped inside a game-editor class
	GamePtr = std::make_shared<ui8>(0);

	// Direct the engine as to where the log should be stored.
	// Log file name will be formatted as "$(WorkingDir)<FileName>_<Timestamp>.log"
	engine::Engine::startLogSystem("DemoGame-Editor");
	{
		// Create the editor object which manages all the behind the scenes stuff.
		auto editor = std::make_shared<Editor>(argc, argv);

		// Add any relevant hooks to relevant systems (like asset editors for assets that are defined by a game)
		editor->OnEngineCreated.bind(&bindEngineHooks);
		editor->OnRegisterAssetEditors.bind(&registerAssetEditors);

		// Actually initializes engine systems
		editor->initialize();
		
		// Setup relevant systems in the editor (rendering, asset editors, build tasks, etc)
		if (editor->setup())
		{
			// Execute whichever editor set needs to run. by default this will open the editor window and run until closed,
			// but this could also just run a process like the build pipeline if the engine was configured with the correct arguments.
			editor->run();
		}
		// By causing the editor to go out of scope, the editor and engine are cleaned up before any final systems (like logging).
	}
	// Ensure the log system ceases and the file is no longer open.
	engine::Engine::stopLogSystem();
	return 0;
}

/**
 * Bind hooks to engine level systems like the asset manager.
 */
void bindEngineHooks(std::shared_ptr<engine::Engine> engine)
{
	engine->getAssetManager()->OnRegisterAssetTypes.bind(GamePtr, &registerAssetTypes);
}

void registerAssetTypes(std::shared_ptr<asset::AssetManager> assetManager)
{
	// STUB: No-op
}

/**
 * Register the relevant asset editors for custom game assets.
 */
void registerAssetEditors(Editor* editor)
{
	// STUB: No-op
}
