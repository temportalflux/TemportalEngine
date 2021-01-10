#pragma once

#include "CoreInclude.hpp"

#include "logging/Logger.hpp"
#include "network/NetworkInterface.hpp"
#include "game/UserIdentity.hpp"

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
	
	network::Interface& networkInterface() { return this->mNetworkInterface; }
	std::shared_ptr<game::WorldLogic> worldLogic() { return this->mpWorldLogic; }
	std::shared_ptr<game::Client> client() { return this->mpClient; }

	//std::shared_ptr<ecs::Entity> localPlayer();

private:
	logging::Logger mProjectLog;

	network::Interface mNetworkInterface;
	std::shared_ptr<game::WorldLogic> mpWorldLogic;
	std::shared_ptr<game::Client> mpClient;

	UserIdentity mLocalUserIdentity;

	std::shared_ptr<ecs::Entity> mpEntityLocalPlayer;
	std::shared_ptr<ecs::system::MovePlayerByInput> mpSystemMovePlayerByInput;
		
	void registerCommands();

	void initializeAssetTypes();

	void registerECSTypes(ecs::Core *ecs);

	//void createLocalPlayer();

	//void bindInput();
	//void unbindInput();
	//void onInputKey(input::Event const& evt);

	void update(f32 deltaTime);

};

NS_END
