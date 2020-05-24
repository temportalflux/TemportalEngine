#include "logging/Logger.hpp"
#include "Engine.hpp"
#include "Window.hpp"
#include "graphics/VulkanRenderer.hpp"

#include <iostream>
#include <string>
#include <stdarg.h>
#include <time.h>

using namespace std;

void initializeNetwork(engine::Engine *pEngine)
{
	char selection;
	string junk;
	cout << "Select (c)lient, (s)erver, or (n)one: ";
	cin >> selection;
	getline(cin, junk);
	switch (selection)
	{
	case 'c':
	case 'C':
	{
		string ip;
		ui16 port;
		cout << "Enter server IP: ";
		getline(cin, ip);
		cout << "Enter port: ";
		cin >> port;
		getline(cin, junk);
		pEngine->createClient(ip.c_str(), port);
		break;
	}
	case 's':
	case 'S':
	{
		ui16 port, maxClients;
		cout << "Enter port: ";
		cin >> port;
		getline(cin, junk);
		cout << "Enter max clients: ";
		cin >> maxClients;
		getline(cin, junk);
		pEngine->createServer(port, maxClients);
		break;
	}
	case 'n':
	case 'N':
	default:
		break;
	}
}

int main()
{
	std::string logFileName = "TemportalEngine_" + logging::LogSystem::getCurrentTimeString() + ".log";
	engine::Engine::LOG_SYSTEM.open(logFileName.c_str());

	engine::Engine *pEngine = engine::Engine::Create();
	LogEngine(logging::ECategory::LOGINFO, "Saving log to %s", logFileName.c_str());

	if (!pEngine->initializeDependencies())
	{
		engine::Engine::Destroy();
		return 1;
	}
	
	initializeNetwork(pEngine);
	
	std::string title = "Demo Game";
	if (pEngine->hasNetwork())
	{
		/*
		auto network = pEngine->getNetworkService();
		if (network.has_value())
		{
			if (network.value()->isServer())
			{
				title += " (Server)";
			}
			else
			{
				title += " (Client)";
			}
		}
		//*/
	}

	utility::SExecutableInfo appInfo = { title.c_str(), TE_MAKE_VERSION(0, 1, 0) };
	pEngine->setApplicationInfo(&appInfo);

	if (!pEngine->setupVulkan())
	{
		engine::Engine::Destroy();
		return 1;
	}

	auto pWindow = pEngine->createWindow(800, 600);
	if (pWindow == nullptr)
	{
		engine::Engine::Destroy();
		return 1;
	}

#pragma region Vulkan
	auto pVulkan = pEngine->initializeVulkan(pWindow->querySDLVulkanExtensions());
	auto renderer = graphics::VulkanRenderer(pVulkan, pWindow->createSurface().initialize(pVulkan));
#pragma endregion

	pEngine->run(pWindow);

	// TODO: Headless https://github.com/temportalflux/ChampNet/blob/feature/final/ChampNet/ChampNetPluginTest/source/StateApplication.cpp#L61

	renderer.invalidate();
	pWindow->destroy();
	engine::Engine::Get()->dealloc(&pWindow);
	engine::Engine::Destroy();

	engine::Engine::LOG_SYSTEM.close();
	return 0;
}