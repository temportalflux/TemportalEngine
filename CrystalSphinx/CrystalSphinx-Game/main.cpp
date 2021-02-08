#include "Engine.hpp"
#include "game/GameInstance.hpp"
#include "logging/Logger.hpp"
#include "utility/FileUtils.hpp"
#include "utility/StringUtils.hpp"
#include "utility/TimeUtils.hpp"
#include "Module.hpp"

void initializePackages();

int main(int argc, char *argv[])
{
	OPTICK_EVENT();
	auto args = utility::parseArguments(argc, argv);

	uSize totalMem = 0;
	auto memoryChunkSizes = utility::parseArgumentInts(args, "memory-", totalMem);

	deleteOldestFiles(std::filesystem::current_path() / "logs", 10);

	std::stringstream ss;
	ss << "logs/";
	ss << utility::currentTimeStr().c_str();
	ss << ".log";
	std::filesystem::path logFileName = ss.str();

	engine::Engine::startLogSystem(logFileName);
	auto logMain = DeclareLog("main", LOG_INFO);

	game::Game::Create(argc, argv);
	initializePackages();

	{
		auto pGame = game::Game::Get();
		if (!pGame->initializeSystems())
		{
			game::Game::Destroy();
			engine::Engine::stopLogSystem();
			return 0;
		}

		pGame->openProject();

		pGame->init();
		pGame->run();
		pGame->uninit();
	}

	game::Game::Destroy();
	engine::Engine::stopLogSystem();
	
	return 0;
}

void initializePackages()
{
	auto packagesDir = std::filesystem::absolute("packages");
	if (std::filesystem::exists(packagesDir))
	{
		// Initialize all dll modules
		for (auto const& entry : std::filesystem::directory_iterator(packagesDir))
		{
			if (!entry.is_directory()) continue;
			auto dllPath = entry.path() / (entry.path().stem().string() + ".dll");
			module_ext::loadModule(dllPath);
		}
		// Initialize all pak assets
		for (auto const& entry : std::filesystem::directory_iterator(packagesDir))
		{
			if (!entry.is_directory()) continue;
			engine::Engine::Get()->loadAssetArchive(entry.path() / (entry.path().stem().string() + ".pak"));
		}
	}
}
