#ifndef TE_ENGINE_HPP
#define TE_ENGINE_HPP

#include "TemportalEnginePCH.hpp"

#include "WindowFlags.hpp"
#include "dependency/SDL.hpp"
#include "graphics/VulkanInstance.hpp"
#include "Delegate.hpp"

#include "ecs/Core.hpp"
#include "input/InputWatcher.hpp"
//#include "network/common/Service.hpp"
#include "thread/Thread.hpp"
#include "utility/Version.hpp"

#include "asset/Project.hpp"

#include "logging/Logger.hpp"
#include "asset/AssetManager.hpp"
#include "ExecutableInfo.hpp"

#include <optional>
#include <unordered_map>
#include <typeinfo>

class Window;
class ITickable;
FORWARD_DEF(NS_COMMAND, class Registry);
FORWARD_DEF(NS_MEMORY, class MemoryChunk);
FORWARD_DEF(NS_INPUT, class Queue);
FORWARD_DEF(NS_GRAPHICS, class VulkanRenderer);

NS_ENGINE

// TODO: Declare log should call a function on engine which lazily instantiates a logger and stores it by category name
#define DeclareLog(title) logging::Logger(title, &engine::Engine::LOG_SYSTEM)
#define LogEngine(cate, ...) DeclareLog("Engine").log(cate, __VA_ARGS__);
#define LogEngineInfo(...) LogEngine(logging::ECategory::LOGINFO, __VA_ARGS__)
#define LogEngineDebug(...) LogEngine(logging::ECategory::LOGDEBUG, __VA_ARGS__)

class TEMPORTALENGINE_API Engine : public std::enable_shared_from_this<Engine>
{
private:
	static std::shared_ptr<Engine> spInstance;
	static std::shared_ptr<memory::MemoryChunk> spMainMemory; // static reference to the main memory object so it doesn't get discarded early

public:
	typedef std::shared_ptr<Engine> EnginePtr;
	static logging::LogSystem LOG_SYSTEM;
	static std::vector<std::string> VulkanValidationLayers;

	static void startLogSystem(std::string const name);
	static void stopLogSystem();

#pragma region Singleton
	static EnginePtr Create(std::unordered_map<std::string, uSize> memoryChunkSizes);
	static EnginePtr Get();
	static void Destroy();
#pragma endregion

	Engine(std::shared_ptr<memory::MemoryChunk> mainMemory, std::unordered_map<std::string, uSize> memoryChunkSizes);
	~Engine();

	void initializeInput();
	std::shared_ptr<input::Queue> getInputQueue() const;

	std::shared_ptr<asset::AssetManager> getAssetManager();

	bool hasProject() const;
	void setProject(asset::ProjectPtrStrong project);
	asset::ProjectPtrStrong getProject() const;

	utility::SExecutableInfo const *const getInfo() const;

#pragma region Memory
	std::shared_ptr<memory::MemoryChunk> getMainMemory() const;
	std::shared_ptr<memory::MemoryChunk> getMiscMemory() const;
#pragma endregion

#pragma region Dependencies
	bool initializeDependencies(bool bShouldRender = true);
	void terminateDependencies();
#pragma endregion

	void initializeECS();
	ecs::Core& getECS();
	ExecuteDelegate<void(ecs::Core* ecs)> ECSRegisterTypesEvent;

#pragma region Windows
	std::shared_ptr<Window> createWindow(ui16 width, ui16 height, std::string title, WindowFlags flags = WindowFlags::RENDER_ON_THREAD);
	void destroyWindow(std::shared_ptr<Window> &pWindow);
#pragma endregion

#pragma region Graphics
	bool setupVulkan();
	std::shared_ptr<graphics::VulkanInstance> initializeVulkan(std::vector<const char*> requiredExtensions);
	void initializeRenderer(graphics::VulkanRenderer *renderer, std::shared_ptr<Window> pWindow);
#pragma endregion

#pragma region Game Loop
	void start();
	bool const isActive() const;
	void update(f32 deltaTime);
	void joinThreads();
	void markShouldStop();
#pragma endregion

	std::shared_ptr<command::Registry> commands();

	void createServer(ui16 const port, ui16 maxClients);
	void createClient(char const *address, ui16 port);
	bool const hasNetwork() const;
	//std::optional<network::Service* const> getNetworkService() const;

	void addTicker(std::weak_ptr<ITickable> tickable);

private:
	asset::ProjectPtrStrong mProject;

	utility::SExecutableInfo mEngineInfo;

#pragma region Memory
	std::shared_ptr<memory::MemoryChunk> mpMainMemory;
	std::shared_ptr<memory::MemoryChunk> mMiscMemory;
#pragma endregion
	
#pragma region Dependencies
	dependency::SDL mpDepSDL[1];
#pragma endregion

	std::shared_ptr<asset::AssetManager> mpAssetManager;

	ecs::Core mECS;
	
#pragma region Windows
	std::map<ui32, std::shared_ptr<Window>> mWindowPtrs;
#pragma endregion
	
#pragma region Graphic
	std::shared_ptr<graphics::VulkanInstance> mpVulkanInstance;
#pragma endregion

#pragma region Input
	input::InputWatcher mpInputWatcher[1];
	std::shared_ptr<input::Queue> mpInputQueue;
#pragma endregion

	std::shared_ptr<command::Registry> mpCommandRegistry;

	bool mbShouldContinueRunning;

	std::vector<std::weak_ptr<ITickable>> mTickers;

	//network::Service *mpNetworkService;

#pragma region Input
	void pollInput();
	void enqueueInput(struct input::Event const &evt);
	void onRawEvent(void *pEvt);
	void processInput(struct input::Event const &evt);
#pragma endregion

};

NS_END

#endif