#ifndef TE_ENGINE_HPP
#define TE_ENGINE_HPP

#include "TemportalEnginePCH.hpp"

#include "WindowFlags.hpp"
#include "dependency/SDL.hpp"
#include "graphics/VulkanInstance.hpp"
#include "input/InputWatcher.hpp"
#include "input/types.h"
//#include "network/common/Service.hpp"
#include "thread/Thread.hpp"
#include "version.h"

#include "asset/Project.hpp"

#include "logging/Logger.hpp"
#include "asset/AssetManager.hpp"
#include "ExecutableInfo.hpp"

#include <optional>
#include <typeinfo>

class Window;
NS_INPUT
class Queue;
NS_END

NS_ENGINE

#define DeclareLog(title) logging::Logger(title, &engine::Engine::LOG_SYSTEM)
#define LogEngine(cate, ...) DeclareLog("Engine").log(cate, __VA_ARGS__);
#define LogEngineInfo(...) LogEngine(logging::ECategory::LOGINFO, __VA_ARGS__)
#define LogEngineDebug(...) LogEngine(logging::ECategory::LOGDEBUG, __VA_ARGS__)

class TEMPORTALENGINE_API Engine
{
private:
	static void* spInstance;

public:
	static logging::LogSystem LOG_SYSTEM;
	static std::vector<const char*> VulkanValidationLayers;

	static constexpr uSize getMaxMemorySize();

#pragma region Singleton
	static Engine* Create();
	static Engine* Get();
	static bool GetChecked(Engine* &instance);
	static void Destroy();
#pragma endregion

	~Engine();

	void setProject(asset::ProjectPtrStrong project);
	bool hasProject() const;
	utility::SExecutableInfo const *const getInfo() const;

#pragma region Memory

	void* getMemoryManager();

	void* allocRaw(uSize size);
	void deallocRaw(void** ptr);

	template <typename TAlloc, typename... TArgs>
	TAlloc* alloc(TArgs... args)
	{
		TAlloc *ptr = (TAlloc*)this->allocRaw(sizeof(TAlloc));
		if (ptr != nullptr)
			new (ptr) TAlloc(args...);
		else
		{
			type_info const &info = typeid(TAlloc);
			LogEngine(logging::ECategory::LOGERROR, "Could not allocate object %s", info.name());
		}
		return ptr;
	}

	template <typename TAlloc, typename... TArgs>
	TAlloc* allocArray(uSize const count, TArgs... args)
	{
		TAlloc *ptr = (TAlloc*)this->allocRaw(sizeof(TAlloc) * count);
		if (ptr != nullptr)
		{
			// TODO: Use std::array::fill
			for (uSize i = 0; i < count; ++i)
				new (&(ptr[i])) TAlloc(args...);
		}
		else
		{
			type_info const &info = typeid(TAlloc);
			LogEngine(logging::ECategory::LOGERROR, "Could not allocate object %s", info.name());
		}
		return ptr;
	}

	template <typename TDealloc>
	void dealloc(TDealloc **ptrRef)
	{
		if (*ptrRef != nullptr)
		{
			(*ptrRef)->TDealloc::~TDealloc();
			this->deallocRaw((void**)ptrRef);
		}
	}

	template <typename TDealloc>
	void deallocArray(uSize const count, TDealloc **ptrRef)
	{
		if (*ptrRef != nullptr)
		{
			for (uSize i = 0; i < count; ++i)
			{
				TDealloc element = (*ptrRef)[i];
				(&element)->TDealloc::~TDealloc();
			}

			this->deallocRaw((void**)ptrRef);
		}
	}

#pragma endregion

#pragma region Dependencies
	bool initializeDependencies();
	void terminateDependencies();
#pragma endregion

	asset::AssetManager* getAssetManager() { return &mAssetManager; }

#pragma region Windows
	Window* createWindow(ui16 width, ui16 height, WindowFlags flags = WindowFlags::RENDER_ON_THREAD);
	void destroyWindow(Window* &pWindow);
#pragma endregion

#pragma region Graphics
	bool setupVulkan();
	graphics::VulkanInstance* initializeVulkan(std::vector<const char*> requiredExtensions);
#pragma endregion

#pragma region Game Loop
	void start();
	bool const isActive() const;
	void update();
	void joinThreads();
	void markShouldStop();
#pragma endregion

	void createServer(ui16 const port, ui16 maxClients);
	void createClient(char const *address, ui16 port);
	bool const hasNetwork() const;
	//std::optional<network::Service* const> getNetworkService() const;

private:
	
	asset::ProjectPtrStrong mProject;

	utility::SExecutableInfo mEngineInfo;

#pragma region Memory
	thread::MutexLock mpLockMemoryManager[1];
	void* mpMemoryManager;
#pragma endregion

#pragma region Dependencies
	dependency::SDL mpDepSDL[1];
#pragma endregion

	asset::AssetManager mAssetManager;
	
#pragma region Windows
	std::map<ui32, Window*> mWindowPtrs;
#pragma endregion
	
#pragma region Graphic
	graphics::VulkanInstance mVulkanInstance;
#pragma endregion

#pragma region Input
	input::InputWatcher mpInputWatcher[1];
	input::Queue *mpInputQueue;
	input::ListenerHandle mInputHandle;
#pragma endregion

	bool mbShouldContinueRunning;

	//network::Service *mpNetworkService;

	Engine(ui32 const & version, void* memoryManager);

#pragma region Input
	void pollInput();
	void enqueueInput(struct input::Event const &evt);
	void onRawEvent(void *pEvt);
	void processInput(struct input::Event const &evt);
#pragma endregion

};

NS_END

#endif