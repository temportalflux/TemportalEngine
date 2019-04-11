#ifndef TE_ENGINE_HPP
#define TE_ENGINE_HPP

#include "TemportalEnginePCH.hpp"

#include "dependency/SDL.hpp"
#include "thread/Thread.hpp"
#include "input/InputWatcher.hpp"
#include "network/common/Service.hpp"
#include <optional>
#include <typeinfo>

#include "logging/Logger.hpp"

class Window;
namespace input
{
	class Queue;
}

NS_ENGINE

#define DeclareLog(title) logging::Logger(title, &engine::Engine::LOG_SYSTEM)
#define LogEngine(cate, ...) logging::Logger("Engine", &engine::Engine::LOG_SYSTEM).log(cate, __VA_ARGS__);
#define LogEngineInfo(...) LogEngine(logging::ECategory::LOGINFO, __VA_ARGS__)
#define LogEngineDebug(...) LogEngine(logging::ECategory::LOGDEBUG, __VA_ARGS__)

//#define MAX_MEMORY_SIZE 2097152 // 8^7

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

	thread::MutexLock mpLockMemoryManager[1];
	void* mpMemoryManager;

	dependency::SDL mpDepSDL[1];

	bool mIsRunning;

	Window *mpWindowGame;
	input::InputWatcher mpInputWatcher[1];

	network::Service *mpNetworkService;
	Thread *mpThreadRender;

	input::Queue *mpInputQueue;

	Engine(void* memoryManager);

public:
	~Engine();

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

	bool const createWindow();
	void destroyWindow();
	bool const hasWindow() const;

	void createServer(ui16 const port, ui16 maxClients);
	void createClient(char const *address, ui16 port);
	
	void run();
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