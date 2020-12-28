#pragma once

#include "CoreInclude.hpp"

#include "logging/Logger.hpp"
#include "input/Event.hpp"
#include "ui/Core.hpp"

class Window;
class Controller;
FORWARD_DEF(NS_ASSET, class AssetManager);

NS_ECS
class Core;
class Entity;
FORWARD_DEF(NS_COMPONENT, class CoordinateTransform);
FORWARD_DEF(NS_SYSTEM, class MovePlayerByInput);
FORWARD_DEF(NS_SYSTEM, class UpdateCameraPerspective);
FORWARD_DEF(NS_SYSTEM, class UpdateDebugHUD);
FORWARD_DEF(NS_SYSTEM, class RenderEntities);
FORWARD_DEF(NS_SYSTEM, class PhysicsIntegration);
NS_END

FORWARD_DEF(NS_GAME, class VoxelModelManager);
FORWARD_DEF(NS_GRAPHICS, class ChunkBoundaryRenderer);
FORWARD_DEF(NS_GRAPHICS, class DescriptorPool);
FORWARD_DEF(NS_GRAPHICS, class EntityInstanceBuffer);
FORWARD_DEF(NS_GRAPHICS, class MinecraftRenderer);
FORWARD_DEF(NS_GRAPHICS, class RenderedString);
FORWARD_DEF(NS_GRAPHICS, class SimpleLineRenderer);
FORWARD_DEF(NS_GRAPHICS, class SkinnedModelManager);
FORWARD_DEF(NS_GRAPHICS, class TextureRegistry);
FORWARD_DEF(NS_GRAPHICS, class UIRenderer);
FORWARD_DEF(NS_GRAPHICS, class VoxelGridRenderer);
FORWARD_DEF(NS_PHYSICS, class Material);
FORWARD_DEF(NS_PHYSICS, class RigidBody);
FORWARD_DEF(NS_PHYSICS, class Scene);
FORWARD_DEF(NS_PHYSICS, class System);
FORWARD_DEF(NS_PHYSICS, class ChunkCollisionManager);
FORWARD_DEF(NS_RESOURCE, class PackManager);
FORWARD_DEF(NS_WORLD, class World);
FORWARD_DEF(NS_WORLD, class BlockInstanceBuffer);
FORWARD_DEF(NS_UI, class TextLogMenu);

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
	void run();
	
	std::shared_ptr<graphics::SkinnedModelManager> modelManager() { return this->mpSkinnedModelManager; }
	std::shared_ptr<graphics::EntityInstanceBuffer> entityInstances() { return this->mpEntityInstanceBuffer; }
	std::shared_ptr<graphics::TextureRegistry> textureRegistry() { return this->mpTextureRegistry; }
	std::shared_ptr<game::VoxelTypeRegistry> voxelTypeRegistry() { return this->mpVoxelTypeRegistry; }

private:
	logging::Logger mProjectLog;

	std::shared_ptr<physics::System> mpPhysics;
	std::shared_ptr<physics::Scene> mpSceneOverworld;
	std::shared_ptr<physics::ChunkCollisionManager> mpChunkCollisionManager; // for Overworld only
	std::shared_ptr<physics::Material> mpPlayerPhysicsMaterial;

#pragma region DemoScene
	std::shared_ptr<physics::Material> mpDefaultPhysMaterial;
	std::shared_ptr<physics::RigidBody> mpBodyPlane;
	std::shared_ptr<physics::RigidBody> mpBodyBall;
#pragma endregion

	std::shared_ptr<resource::PackManager> mpResourcePackManager;

	std::shared_ptr<Window> mpWindow;
	std::shared_ptr<graphics::MinecraftRenderer> mpRenderer;
	std::shared_ptr<world::BlockInstanceBuffer> mpVoxelInstanceBuffer;
	std::shared_ptr<graphics::VoxelGridRenderer> mpVoxelGridRenderer;
	std::shared_ptr<graphics::SimpleLineRenderer> mpWorldAxesRenderer;
	std::shared_ptr<graphics::ChunkBoundaryRenderer> mpChunkBoundaryRenderer;
	std::shared_ptr<graphics::UIRenderer> mpUIRenderer;
	std::shared_ptr<graphics::SkinnedModelManager> mpSkinnedModelManager;
	std::shared_ptr<graphics::EntityInstanceBuffer> mpEntityInstanceBuffer;
	std::shared_ptr<ecs::system::RenderEntities> mpSystemRenderEntities;
	std::shared_ptr<graphics::TextureRegistry> mpTextureRegistry;
	std::shared_ptr<ui::TextLogMenu> mpMenuTextLog;

	std::shared_ptr<ecs::Entity> mpEntityLocalPlayer;
	std::shared_ptr<ecs::system::MovePlayerByInput> mpSystemMovePlayerByInput;
	std::shared_ptr<ecs::system::UpdateCameraPerspective> mpSystemUpdateCameraPerspective;
	std::shared_ptr<ecs::system::UpdateDebugHUD> mpSystemUpdateDebugHUD;
	std::shared_ptr<ecs::system::PhysicsIntegration> mpSystemPhysicsIntegration;

	std::shared_ptr<game::VoxelTypeRegistry> mpVoxelTypeRegistry;
	std::shared_ptr<game::VoxelModelManager> mpVoxelModelManager;

	std::shared_ptr<world::World> mpWorld;
		
	void initializeAssetTypes();
	void destroyWindow();

	void registerECSTypes(ecs::Core *ecs);

	void createVoxelTypeRegistry();
	void destroyVoxelTypeRegistry();

	bool requiresGraphics() const;
	bool initializeGraphics();
	bool createWindow();

	void createRenderers();
	void createGameRenderer();
	void loadVoxelTypeTextures();
	void createPipelineRenderers();
	void createVoxelGridRenderer();
	void createWorldAxesRenderer();
	void createChunkBoundaryRenderer();
	void createUIRenderer();
	void destroyRenderers();

	bool scanResourcePacks();
	void destroyResourcePacks();

	void createScene();
	void destroyScene();
	void createWorld();
	void destroyWorld();
	void createLocalPlayer();

	void bindInput();
	void unbindInput();
	void onInputKey(input::Event const& evt);

	void update(f32 deltaTime);
	void updateWorldGraphics();

};

NS_END
