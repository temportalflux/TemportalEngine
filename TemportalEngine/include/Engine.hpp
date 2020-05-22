#ifndef TE_ENGINE_HPP
#define TE_ENGINE_HPP

#include "TemportalEnginePCH.hpp"

#include <optional>
#include <typeinfo>

#include "dependency/SDL.hpp"
#include "input/InputWatcher.hpp"
#include "input/types.h"
#include "network/common/Service.hpp"
#include "thread/Thread.hpp"

#include "logging/Logger.hpp"
#include "ExecutableInfo.hpp"

class Window;
NS_INPUT
class Queue;
NS_END

NS_ENGINE

#define DeclareLog(title) logging::Logger(title, &engine::Engine::LOG_SYSTEM)
#define LogEngine(cate, ...) DeclareLog("Engine").log(cate, __VA_ARGS__);
#define LogEngineInfo(...) LogEngine(logging::ECategory::LOGINFO, __VA_ARGS__)
#define LogEngineDebug(...) LogEngine(logging::ECategory::LOGDEBUG, __VA_ARGS__)

// Creates a unique 32-bit integer version for a unique semantic version
// NOTE: based on vulkan's VK_MAKE_VERSION
#define TE_MAKE_VERSION(major, minor, patch) (((major) << 22) | ((minor) << 12) | (patch))
#define TE_GET_MAJOR_VERSION(version) ((ui32)(version) >> 22)
#define TE_GET_MINOR_VERSION(version) (((ui32)(version) >> 12) & 0x3ff)
#define TE_GET_PATCH_VERSION(version) ((ui32)(version) & 0xfff)

class TEMPORTALENGINE_API Engine
{
private:
	static void* spInstance;

public:
	static logging::LogSystem LOG_SYSTEM;

	static constexpr uSize getMaxMemorySize();
	static Engine* Create();
	static Engine* Get();
	static bool GetChecked(Engine* &instance);
	static void Destroy();

private:

	utility::SExecutableInfo mInfo;

	thread::MutexLock mpLockMemoryManager[1];
	void* mpMemoryManager;

	dependency::SDL mpDepSDL[1];

	bool mIsRunning;

	input::InputWatcher mpInputWatcher[1];

	network::Service *mpNetworkService;
	Thread *mpThreadRender;

	input::Queue *mpInputQueue;
	input::ListenerHandle mInputHandle;

	Engine(ui32 const & version, void* memoryManager);

public:
	~Engine();
	utility::SExecutableInfo const *const getInfo() const;

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

	bool initializeDependencies();
	void terminateDependencies();

	Window* createWindow(utility::SExecutableInfo const *const pAppInfo);

	void createServer(ui16 const port, ui16 maxClients);
	void createClient(char const *address, ui16 port);
	
	void run(Window* pWindow);
	bool const isActive() const;
	void markShouldStop();

	bool const hasNetwork() const;
	std::optional<network::Service* const> getNetworkService() const;

	void pollInput();
	void enqueueInput(struct input::Event const &evt);
	void processInput(struct input::Event const &evt);

};

NS_END

#endif