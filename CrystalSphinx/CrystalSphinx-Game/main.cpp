#include "Engine.hpp"
#include "game/GameInstance.hpp"
#include "logging/Logger.hpp"
#include "utility/StringUtils.hpp"
#include "utility/TimeUtils.hpp"
#include "Module.hpp"

int main(int argc, char *argv[])
{
	OPTICK_EVENT();
	auto args = utility::parseArguments(argc, argv);

	uSize totalMem = 0;
	auto memoryChunkSizes = utility::parseArgumentInts(args, "memory-", totalMem);

	std::stringstream ss;
	ss << "logs/";
	ss << utility::currentTimeStr().c_str();
	ss << ".log";
	std::filesystem::path logFileName = ss.str();

	engine::Engine::startLogSystem(logFileName);
	auto logMain = DeclareLog("main");

	auto modulesDir = std::filesystem::absolute("Modules");
	if (std::filesystem::exists(modulesDir))
	{
		for (auto const& entry : std::filesystem::directory_iterator(modulesDir))
		{
			if (!entry.is_directory()) continue;
			auto dllPath = entry.path() / (entry.path().stem().string() + ".dll");
			module_ext::loadModule(dllPath);
		}
	}

	{
		auto pGame = game::Game::Create(argc, argv);
		if (pGame->initializeSystems())
		{
			pGame->openProject();
			pGame->init();
			pGame->run();
			pGame->uninit();
		}
	}
	game::Game::Destroy();
	engine::Engine::stopLogSystem();
	
	return 0;
}