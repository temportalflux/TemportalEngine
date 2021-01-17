#pragma once

#include "CoreInclude.hpp"

#include "dataStructures/Singleton.hpp"
#include "logging/Logger.hpp"
#include "network/NetworkCore.hpp"
#include "network/NetworkAddress.hpp"
#include "saveData/SaveDataRegistry.hpp"
#include "settings/ServerSettings.hpp"
#include "utility/Flags.hpp"
#include "utility/Guid.hpp"

class Controller;
FORWARD_DEF(NS_ASSET, class AssetManager);

NS_ECS
class Core;
class Entity;
FORWARD_DEF(NS_SYSTEM, class MovePlayerByInput);
FORWARD_DEF(NS_SYSTEM, class PhysicsIntegration);
NS_END

FORWARD_DEF(NS_GAME, class Client);
FORWARD_DEF(NS_GAME, class Server);
FORWARD_DEF(NS_GAME, class World);
FORWARD_DEF(NS_NETWORK, class Interface);

NS_GAME

class Game : public Singleton<Game, int, char*[]>
{

public:
	static std::shared_ptr<asset::AssetManager> assetManager();
	
	Game(int argc, char *argv[]);
	~Game();

	bool initializeSystems();
	void openProject();
	void init();
	void uninit();
	void run();

	static network::Interface* networkInterface();
	std::shared_ptr<game::World> worldLogic();
	std::shared_ptr<game::Server> server();
	std::shared_ptr<game::Client> client();
	saveData::Registry& saveData();

	//std::shared_ptr<ecs::Entity> localPlayer();

	void setupNetworkServer(utility::Flags<network::EType> flags);

private:
	logging::Logger mProjectLog;

	std::shared_ptr<game::World> mpWorldLogic;
	std::shared_ptr<game::Server> mpServer;
	std::shared_ptr<game::Client> mpClient;

	saveData::Registry mSaveDataRegistry;

	std::shared_ptr<ecs::Entity> mpEntityLocalPlayer;
	std::shared_ptr<ecs::system::MovePlayerByInput> mpSystemMovePlayerByInput;
		
	void registerCommands();
	void initializeAssetTypes();
	void registerECSTypes(ecs::Core *ecs);

	void onNetworkConnectionOpened(network::Interface *pInterface, ui32 connection, ui32 netId);

	//void createLocalPlayer();

	//void bindInput();
	//void unbindInput();
	//void onInputKey(input::Event const& evt);

	void update(f32 deltaTime);

};

NS_END
