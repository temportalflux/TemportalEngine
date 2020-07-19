#include "Engine.hpp"
#include "game/Game.hpp"
#include "logging/Logger.hpp"
#include "utility/StringUtils.hpp"

int main(int argc, char *argv[])
{
	auto args = utility::parseArguments(argc, argv);

	uSize totalMem = 0;
	auto memoryChunkSizes = utility::parseArgumentInts(args, "memory-", totalMem);

	std::string logFileName = "DemoGame_" + logging::LogSystem::getCurrentTimeString() + ".log";
	engine::Engine::LOG_SYSTEM.open(logFileName.c_str());
	auto logMain = logging::Logger("main", &engine::Engine::LOG_SYSTEM);
	logMain.log(logging::ECategory::LOGINFO, "Saving log to %s", logFileName.c_str());

	{
		auto pGame = game::Game::Create(argc, argv);
		if (pGame->initializeSystems())
		{
			pGame->openProject();
			pGame->initializeNetwork();
			if (!pGame->requiresGraphics() || (pGame->initializeGraphics() && pGame->createWindow()))
			{
				if (pGame->requiresGraphics())
				{
					pGame->createRenderers();
				}

				pGame->createScene();
				pGame->run();
				pGame->destroyScene();

				if (pGame->requiresGraphics())
				{
					pGame->destroyRenderers();
				}
			}
		}
	}
	game::Game::Destroy();
	logMain.log(logging::ECategory::LOGINFO, "Closing log. File can be found at %s", logFileName.c_str());
	engine::Engine::LOG_SYSTEM.close();
	
	return 0;
}