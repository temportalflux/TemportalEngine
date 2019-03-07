#include "logging/Logger.hpp"
#include "Engine.hpp"

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
	string logFile = "TemportalEngine_";
	{
		time_t currentTime = time(nullptr);
		struct tm timeinfo;
		localtime_s(&timeinfo, &currentTime);
		char timeStr[70];
		strftime(timeStr, sizeof(timeStr), "%Y-%m-%d-%H-%M-%S", &timeinfo);
		logFile.append(timeStr);
	}
	logFile += ".log";

	engine::Engine::LOG_SYSTEM.open(logFile.c_str());

	engine::Engine *pEngine = engine::Engine::Create();
	if (!pEngine->initializeDependencies()) return 1;
	
	LogEngine(logging::ECategory::LOGINFO, "Saving log to %s", logFile.c_str());

	initializeNetwork(pEngine);

	if (!pEngine->createWindow())
	{
		engine::Engine::Destroy();
		return 1;
	}

	pEngine->run();

	// TODO: Headless https://github.com/temportalflux/ChampNet/blob/feature/final/ChampNet/ChampNetPluginTest/source/StateApplication.cpp#L61

	engine::Engine::Destroy();

	engine::Engine::LOG_SYSTEM.close();
	return 0;
}