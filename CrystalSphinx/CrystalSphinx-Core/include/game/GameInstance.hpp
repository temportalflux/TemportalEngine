#pragma once

#include "CoreInclude.hpp"

#include "dataStructures/Singleton.hpp"
#include "logging/Logger.hpp"
#include "network/NetworkCore.hpp"
#include "network/NetworkAddress.hpp"
#include "saveData/SaveDataRegistry.hpp"
#include "saveData/ServerSettings.hpp"
#include "utility/Flags.hpp"
#include "utility/Guid.hpp"

class Controller;
FORWARD_DEF(NS_ASSET, class AssetManager);

NS_EVCS
class Core;
class Entity;
FORWARD_DEF(NS_SYSTEM, class MovePlayerByInput);
FORWARD_DEF(NS_SYSTEM, class PhysicsIntegration);
NS_END

FORWARD_DEF(NS_GAME, class Client);
FORWARD_DEF(NS_GAME, class Server);
FORWARD_DEF(NS_NETWORK, class Interface);
FORWARD_DEF(NS_SAVE_DATA, class Instance);
FORWARD_DEF(NS_WORLD, class World);

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
	std::shared_ptr<game::Server> server();
	void destroyServer();
	std::shared_ptr<game::Client> client();
	saveData::Registry& saveData();
	std::shared_ptr<world::World> world();
	void destroyWorld();

	void setupNetworkServer(
		utility::Flags<network::EType> flags,
		saveData::Instance *saveInstance
	);

	template <typename TWorld, typename... TArgs>
	std::shared_ptr<TWorld> createWorld(TArgs... args)
	{
		assert(!this->mpWorld);
		this->mpWorld = std::make_shared<TWorld>(args...);
		return std::dynamic_pointer_cast<TWorld>(this->mpWorld);
	}

private:
	logging::Logger mProjectLog;

	std::optional<std::string> mServerSaveId;
	std::shared_ptr<game::Server> mpServer;
	std::shared_ptr<game::Client> mpClient;
	saveData::Registry mSaveDataRegistry;
	std::shared_ptr<world::World> mpWorld;

	std::shared_ptr<evcs::Entity> mpEntityLocalPlayer;
	std::shared_ptr<evcs::system::MovePlayerByInput> mpSystemMovePlayerByInput;
		
	void registerCommands();
	void initializeAssetTypes();
	void registerECSTypes(evcs::Core *ecs);

	void update(f32 deltaTime);

};

NS_END
