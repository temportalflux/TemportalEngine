#pragma once

#include "CoreInclude.hpp"

class Window;
class RenderCube;
class Controller;
FORWARD_DEF(NS_ASSET, class AssetManager);
FORWARD_DEF(NS_ECS, struct ComponentTransform);
FORWARD_DEF(NS_GRAPHICS, class GameRenderer);
FORWARD_DEF(NS_GRAPHICS, class RenderedString);
FORWARD_DEF(NS_GRAPHICS, class Uniform);

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
	void initializeNetwork();

	bool requiresGraphics() const;
	bool initializeGraphics();
	bool createWindow();
	
	void createRenderers();
	void destroyRenderers();

	void createScene();
	void destroyScene();

	void run();

private:
	std::shared_ptr<Window> mpWindow;
	std::shared_ptr<graphics::GameRenderer> mpRenderer;
	std::shared_ptr<graphics::Uniform> mpRendererMVP;
	std::shared_ptr<RenderCube> mpCubeRender;
	std::weak_ptr<graphics::RenderedString> mpCameraForwardStr;

	std::shared_ptr<Controller> mpController;
	std::shared_ptr<ecs::ComponentTransform> mpCameraTransform;
		
	void initializeAssetTypes();
	void destroyWindow();

};

NS_END
