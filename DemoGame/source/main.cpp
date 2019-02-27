#include "logging/Logger.hpp"
#include "Engine.hpp"

using namespace std;

int main()
{
	engine::Engine::LOG_SYSTEM.open("TemportalEngine.log");

	engine::Engine *pEngine = engine::Engine::Create();
	
	if (!pEngine->initializeDependencies()) return 1;

	if (!pEngine->createWindow())
	{
		engine::Engine::Destroy();
		return 1;
	}

	pEngine->createServer(425, 2);

	pEngine->run();

	engine::Engine::Destroy();

	engine::Engine::LOG_SYSTEM.close();
	return 0;
}