#pragma once

#include "CoreInclude.hpp"

#include "logging/Logger.hpp"
#include "game/UserIdentity.hpp"
#include "network/NetworkCore.hpp"
#include "network/NetworkAddress.hpp"
#include "settings/ServerSettings.hpp"
#include "settings/UserSettings.hpp"
#include "utility/Flags.hpp"

class Controller;
FORWARD_DEF(NS_ASSET, class AssetManager);

NS_ECS
class Core;
class Entity;
FORWARD_DEF(NS_SYSTEM, class MovePlayerByInput);
FORWARD_DEF(NS_SYSTEM, class PhysicsIntegration);
NS_END

FORWARD_DEF(NS_GAME, class Client);
FORWARD_DEF(NS_GAME, class WorldLogic);
FORWARD_DEF(NS_NETWORK, class Interface);

NS_GAME

class Game : public std::enable_shared_from_this<Game>
{

#pragma region Singleton
public:
	static std::shared_ptr<Game> Create(int argc, char *argv[]);
	static std::shared_ptr<Game> Get();
	static void Destroy();
private:
	static std::shared_ptr<Game> gpInstance;
#pragma endregion

public:
	static std::shared_ptr<asset::AssetManager> assetManager();
	
	Game(int argc, char *argv[]);
	~Game();

	bool initializeSystems();
	void openProject();
	void init();
	void uninit();
	void run();
	
	void setLocalUserNetId(ui32 netId);
	game::UserIdentity& localUser();
	game::UserIdentity& findConnectedUser(ui32 netId);
	void removeConnectedUser(ui32 netId);

	static network::Interface* networkInterface();
	std::shared_ptr<game::WorldLogic> worldLogic() { return this->mpWorldLogic; }
	std::shared_ptr<game::Client> client() { return this->mpClient; }

	//std::shared_ptr<ecs::Entity> localPlayer();

private:
	logging::Logger mProjectLog;

	game::ServerSettings mServerSettings;
	game::UserSettings mUserSettings;

	std::shared_ptr<game::WorldLogic> mpWorldLogic;
	std::shared_ptr<game::Client> mpClient;

	bool mbHasLocalUserNetId;
	ui32 mLocalUserNetId;
	std::map<ui32, game::UserIdentity> mConnectedUsers;

	std::shared_ptr<ecs::Entity> mpEntityLocalPlayer;
	std::shared_ptr<ecs::system::MovePlayerByInput> mpSystemMovePlayerByInput;
		
	void registerCommands();

	void addConnectedUser(ui32 netId);

	void initializeAssetTypes();

	void registerECSTypes(ecs::Core *ecs);

	void startDedicatedServer();
	void startDedicatedClient(network::Address const& serverAddress);
	void startIntegratedClientServer();

	//void createLocalPlayer();

	//void bindInput();
	//void unbindInput();
	//void onInputKey(input::Event const& evt);

	void update(f32 deltaTime);

};

NS_END
