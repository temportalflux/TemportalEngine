#include "GameEditor.hpp"
#include "Engine.hpp"
#include "asset/AssetManager.hpp"
#include "asset/TypedAssetPath.hpp"
#include "asset/Project.hpp"

int main(int argc, char *argv[])
{
	// Direct the engine as to where the log should be stored.
	// Log file name will be formatted as "$(WorkingDir)<FileName>_<Timestamp>.log"
	engine::Engine::startLogSystem("Editor");
	{
		// Create the editor object which manages all the behind the scenes stuff.
		auto editor = GameEditor(argc, argv);

		// Actually initializes engine systems
		editor.initialize();
		
		// Setup relevant systems in the editor (rendering, asset editors, build tasks, etc)
		if (editor.setup())
		{
			// Attempt to auto-load with where we think the project file is
			{
				auto projPath = asset::TypedAssetPath<asset::Project>::Create(
					std::filesystem::relative("../../Minecraft/Minecraft.te-project", std::filesystem::current_path())
				);
				if (projPath.path().isValid())
				{
					editor.setProject(projPath.load(asset::EAssetSerialization::Json));
				}
			}

			// Execute whichever editor set needs to run. by default this will open the editor window and run until closed,
			// but this could also just run a process like the build pipeline if the engine was configured with the correct arguments.
			editor.run();
		}
		// By causing the editor to go out of scope, the editor and engine are cleaned up before any final systems (like logging).
	}
	// Ensure the log system ceases and the file is no longer open.
	engine::Engine::stopLogSystem();
	return 0;
}
