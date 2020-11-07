#pragma once

#include "CoreInclude.hpp"

#include "logging/Logger.hpp"

class Window;
class Controller;
FORWARD_DEF(NS_ASSET, class AssetManager);
FORWARD_DEF(NS_ECS, struct ComponentTransform);
FORWARD_DEF(NS_GAME, class VoxelModelManager);
FORWARD_DEF(NS_GRAPHICS, class DescriptorPool);
FORWARD_DEF(NS_GRAPHICS, class MinecraftRenderer);
FORWARD_DEF(NS_GRAPHICS, class RenderedString);
FORWARD_DEF(NS_GRAPHICS, class VoxelGridRenderer);
FORWARD_DEF(NS_GRAPHICS, class WorldAxesRenderer);
FORWARD_DEF(NS_GRAPHICS, class Uniform);
FORWARD_DEF(NS_WORLD, class World);
FORWARD_DEF(NS_WORLD, class BlockInstanceBuffer);

NS_GAME
class VoxelTypeRegistry;

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
	void initializeNetwork();
	void init();
	void uninit();

	bool requiresGraphics() const;
	bool initializeGraphics();
	bool createWindow();
	
	void createRenderers();
	void destroyRenderers();

	void createScene();
	void destroyScene();

	void run();

private:
	logging::Logger mProjectLog;

	std::shared_ptr<Window> mpWindow;
	std::shared_ptr<graphics::DescriptorPool> mpGlobalDescriptorPool;
	std::shared_ptr<graphics::MinecraftRenderer> mpRenderer;
	std::shared_ptr<graphics::Uniform> mpRendererMVP;
	std::shared_ptr<graphics::Uniform> mpUniformLocalCamera;
	std::shared_ptr<world::BlockInstanceBuffer> mpVoxelInstanceBuffer;
	std::shared_ptr<graphics::VoxelGridRenderer> mpVoxelGridRenderer;
	std::shared_ptr<graphics::WorldAxesRenderer> mpWorldAxesRenderer;
	//std::weak_ptr<graphics::RenderedString> mpCameraForwardStr;

	std::shared_ptr<Controller> mpController;
	std::shared_ptr<ecs::ComponentTransform> mpCameraTransform;

	std::shared_ptr<game::VoxelTypeRegistry> mpVoxelTypeRegistry;
	std::shared_ptr<game::VoxelModelManager> mpVoxelModelManager;

	std::shared_ptr<world::World> mpWorld;
		
	void initializeAssetTypes();
	void destroyWindow();

	void createVoxelTypeRegistry();
	void destroyVoxelTypeRegistry();

	void createGameRenderer();
	void loadVoxelTypeTextures();
	void createPipelineRenderers();
	void createVoxelGridRenderer();
	void createWorldAxesRenderer();

	void update(f32 deltaTime);
	void updateCameraUniform();

};

NS_END
