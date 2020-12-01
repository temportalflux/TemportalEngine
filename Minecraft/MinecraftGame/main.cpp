#include "Engine.hpp"
#include "Game.hpp"
#include "logging/Logger.hpp"
#include "utility/StringUtils.hpp"
#include "Module.hpp"

int main(int argc, char *argv[])
{
	OPTICK_EVENT();
	auto args = utility::parseArguments(argc, argv);

	uSize totalMem = 0;
	auto memoryChunkSizes = utility::parseArgumentInts(args, "memory-", totalMem);

	std::string logFileName = "DemoGame_" + logging::LogSystem::getCurrentTimeString() + ".log";
	engine::Engine::LOG_SYSTEM.open(logFileName.c_str());
	auto logMain = logging::Logger("main", &engine::Engine::LOG_SYSTEM);
	logMain.log(logging::ECategory::LOGINFO, "Saving log to %s", logFileName.c_str());

	for (auto const& entry : std::filesystem::directory_iterator(std::filesystem::absolute("Modules")))
	{
		if (!entry.is_directory()) continue;
		auto dllPath = entry.path() / (entry.path().stem().string() + ".dll");
		module_ext::loadModule(dllPath);
	}

	{
		auto pGame = game::Game::Create(argc, argv);
		if (pGame->initializeSystems())
		{
			pGame->openProject();
			pGame->initializeNetwork();
			pGame->init();
			pGame->run();
			pGame->uninit();
		}
	}
	game::Game::Destroy();
	logMain.log(logging::ECategory::LOGINFO, "Closing log. File can be found at %s", logFileName.c_str());
	engine::Engine::LOG_SYSTEM.close();
	
	return 0;
}