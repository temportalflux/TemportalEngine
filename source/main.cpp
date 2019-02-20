#include "Log.h"
#include "Engine.hpp"

using namespace std;

#define LogEngine "TemportalEngine"

int main()
{
	LogEngineInfo("Hello World!");

	engine::Engine *pEngine = engine::Engine::Create();
	
	if (!pEngine->initializeDependencies()) return 1;

	if (!pEngine->createWindow())
	{
		engine::Engine::Destroy();
		return 1;
	}

	pEngine->run();

	engine::Engine::Destroy();

	//system("pause");
	return 0;
}